#include <tiny_os/process/scheduler.h>
#include <tiny_os/process/process.h>
#include <tiny_os/process/context_switch.h>
#include <tiny_os/arch/x86_64/idt.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/common/string.h>

namespace tiny_os::process {

// Static member definitions
Thread* Scheduler::ready_queue_[MAX_READY_THREADS];
usize Scheduler::ready_queue_head_ = 0;
usize Scheduler::ready_queue_tail_ = 0;
usize Scheduler::ready_queue_size_ = 0;
Thread* Scheduler::current_thread_ = nullptr;
Thread* Scheduler::idle_thread_ = nullptr;
bool Scheduler::scheduling_enabled_ = false;
uint64 Scheduler::context_switches_ = 0;
uint64 Scheduler::idle_time_ = 0;

// Idle thread function
static void idle_thread_func() {
    while (true) {
        asm volatile("hlt");
    }
}

void Scheduler::init() {
    drivers::serial_printf("[Scheduler] Initializing scheduler...\n");

    // Clear ready queue
    ready_queue_head_ = 0;
    ready_queue_tail_ = 0;
    ready_queue_size_ = 0;

    context_switches_ = 0;
    idle_time_ = 0;
    scheduling_enabled_ = false;

    drivers::serial_printf("[Scheduler] Scheduler initialized\n");
    drivers::kprintf("[Scheduler] Scheduler initialized\n");
}

void Scheduler::start() {
    drivers::serial_printf("[Scheduler] Starting scheduler...\n");

    // Create idle process and thread
    Process* idle_process = ProcessManager::create_kernel_process("idle", idle_thread_func);
    if (!idle_process) {
        drivers::serial_printf("[Scheduler] Failed to create idle process!\n");
        return;
    }

    idle_thread_ = idle_process->main_thread;
    idle_thread_->priority = 0;  // Lowest priority

    // Set current thread to idle
    current_thread_ = idle_thread_;
    current_thread_->state = ThreadState::RUNNING;
    ThreadManager::set_current(current_thread_);

    // Enable scheduling
    scheduling_enabled_ = true;

    drivers::serial_printf("[Scheduler] Scheduler started with idle thread %d\n",
                          idle_thread_->tid);
    drivers::kprintf("[Scheduler] Scheduler started\n");
}

void Scheduler::add_thread(Thread* thread) {
    if (!thread) return;

    drivers::serial_printf("[Scheduler] Adding thread %d (%s) to ready queue\n",
                          thread->tid, thread->name);

    // Disable interrupts while modifying queue
    bool interrupts_enabled = arch::x86_64::IDT::are_interrupts_enabled();
    if (interrupts_enabled) {
        arch::x86_64::IDT::disable_interrupts();
    }

    if (ready_queue_size_ >= MAX_READY_THREADS) {
        drivers::serial_printf("[Scheduler] Ready queue full! Cannot add thread %d\n",
                              thread->tid);
        if (interrupts_enabled) {
            arch::x86_64::IDT::enable_interrupts();
        }
        return;
    }

    // Add to tail of queue
    ready_queue_[ready_queue_tail_] = thread;
    ready_queue_tail_ = (ready_queue_tail_ + 1) % MAX_READY_THREADS;
    ready_queue_size_++;

    thread->state = ThreadState::READY;

    if (interrupts_enabled) {
        arch::x86_64::IDT::enable_interrupts();
    }

    drivers::serial_printf("[Scheduler] Thread %d added (queue size: %d)\n",
                          thread->tid, ready_queue_size_);
}

void Scheduler::remove_thread(Thread* thread) {
    if (!thread) return;

    // Disable interrupts while modifying queue
    bool interrupts_enabled = arch::x86_64::IDT::are_interrupts_enabled();
    if (interrupts_enabled) {
        arch::x86_64::IDT::disable_interrupts();
    }

    // Find and remove thread from queue
    usize index = ready_queue_head_;
    for (usize i = 0; i < ready_queue_size_; i++) {
        if (ready_queue_[index] == thread) {
            // Shift remaining threads
            for (usize j = i; j < ready_queue_size_ - 1; j++) {
                usize curr = (ready_queue_head_ + j) % MAX_READY_THREADS;
                usize next = (ready_queue_head_ + j + 1) % MAX_READY_THREADS;
                ready_queue_[curr] = ready_queue_[next];
            }

            ready_queue_size_--;
            if (ready_queue_size_ > 0) {
                ready_queue_tail_ = (ready_queue_head_ + ready_queue_size_) % MAX_READY_THREADS;
            } else {
                ready_queue_tail_ = ready_queue_head_;
            }

            drivers::serial_printf("[Scheduler] Removed thread %d from ready queue\n",
                                  thread->tid);
            break;
        }

        index = (index + 1) % MAX_READY_THREADS;
    }

    if (interrupts_enabled) {
        arch::x86_64::IDT::enable_interrupts();
    }
}

void Scheduler::schedule() {
    if (!scheduling_enabled_) return;

    // Get next thread to run
    Thread* next = get_next_thread();

    // If next is same as current, nothing to do
    if (next == current_thread_) {
        return;
    }

    // Switch to next thread
    switch_to(next);
}

void Scheduler::yield() {
    if (!scheduling_enabled_) return;

    drivers::serial_printf("[Scheduler] Thread %d yielding\n",
                          current_thread_ ? current_thread_->tid : 0);

    schedule();
}

void Scheduler::block_current() {
    if (!current_thread_) return;

    drivers::serial_printf("[Scheduler] Blocking thread %d\n", current_thread_->tid);

    current_thread_->state = ThreadState::BLOCKED;
    remove_thread(current_thread_);
    yield();
}

void Scheduler::unblock_thread(Thread* thread) {
    if (!thread) return;

    drivers::serial_printf("[Scheduler] Unblocking thread %d\n", thread->tid);

    thread->state = ThreadState::READY;
    add_thread(thread);
}

Thread* Scheduler::current_thread() {
    return current_thread_;
}

void Scheduler::print_stats() {
    drivers::kprintf("\n=== Scheduler Statistics ===\n");
    drivers::kprintf("Context switches: %d\n", context_switches_);
    drivers::kprintf("Idle time: %d ticks\n", idle_time_);
    drivers::kprintf("Ready queue size: %d\n", ready_queue_size_);
    drivers::kprintf("Current thread: %d (%s)\n",
                    current_thread_ ? current_thread_->tid : 0,
                    current_thread_ ? current_thread_->name : "none");
    drivers::kprintf("\n");
}

Thread* Scheduler::get_next_thread() {
    // If ready queue is empty, return idle thread
    if (ready_queue_size_ == 0) {
        return idle_thread_;
    }

    // Get thread from head of queue
    Thread* next = ready_queue_[ready_queue_head_];
    ready_queue_head_ = (ready_queue_head_ + 1) % MAX_READY_THREADS;
    ready_queue_size_--;

    // If current thread is still ready, add it back to queue
    if (current_thread_ &&
        current_thread_->state == ThreadState::RUNNING &&
        current_thread_ != idle_thread_) {
        add_thread(current_thread_);
    }

    return next;
}

void Scheduler::switch_to(Thread* next_thread) {
    if (!next_thread || next_thread == current_thread_) {
        return;
    }

    Thread* old_thread = current_thread_;

    drivers::serial_printf("[Scheduler] Context switch: %d (%s) -> %d (%s)\n",
                          old_thread ? old_thread->tid : 0,
                          old_thread ? old_thread->name : "none",
                          next_thread->tid,
                          next_thread->name);

    // Update states
    if (old_thread && old_thread->state == ThreadState::RUNNING) {
        old_thread->state = ThreadState::READY;
    }

    next_thread->state = ThreadState::RUNNING;
    current_thread_ = next_thread;
    ThreadManager::set_current(next_thread);

    context_switches_++;

    // Track idle time
    if (next_thread == idle_thread_) {
        idle_time_++;
    }

    // Perform context switch
    if (old_thread) {
        context_switch(&old_thread->cpu_state, next_thread->cpu_state);
    } else {
        // First time - just load new context
        // This shouldn't normally happen
        drivers::serial_printf("[Scheduler] WARNING: First context switch with null old_thread\n");
    }
}

} // namespace tiny_os::process
