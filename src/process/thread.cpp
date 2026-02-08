#include <tiny_os/process/thread.h>
#include <tiny_os/process/process.h>
#include <tiny_os/process/scheduler.h>
#include <tiny_os/memory/heap_allocator.h>
#include <tiny_os/memory/virtual_allocator.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/common/string.h>

namespace tiny_os::process {

// Static member definitions
uint32 ThreadManager::next_tid_ = 1;
Thread* ThreadManager::current_thread_ = nullptr;

const char* thread_state_to_string(ThreadState state) {
    switch (state) {
        case ThreadState::CREATED: return "CREATED";
        case ThreadState::READY: return "READY";
        case ThreadState::RUNNING: return "RUNNING";
        case ThreadState::BLOCKED: return "BLOCKED";
        case ThreadState::TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

void ThreadManager::init() {
    drivers::serial_printf("[Thread] Initializing thread manager...\n");
    drivers::serial_printf("[Thread] Thread manager initialized\n");
    drivers::kprintf("[Thread] Thread manager initialized\n");
}

Thread* ThreadManager::create_kernel_thread(Process* process, const char* name,
                                           void (*entry_point)()) {
    drivers::serial_printf("[Thread] Creating kernel thread: %s\n", name);

    // Allocate TCB
    Thread* thread = new Thread();
    if (!thread) {
        drivers::serial_printf("[Thread] Failed to allocate TCB!\n");
        return nullptr;
    }

    // Initialize TCB
    thread->tid = allocate_tid();
    thread->process = process;
    thread->state = ThreadState::CREATED;

    // Allocate kernel stack
    thread->stack_size = DEFAULT_STACK_SIZE;
    void* stack_memory = memory::HeapAllocator::kmalloc(thread->stack_size);
    if (!stack_memory) {
        drivers::serial_printf("[Thread] Failed to allocate stack!\n");
        delete thread;
        return nullptr;
    }

    thread->kernel_stack_bottom = reinterpret_cast<VirtualAddress>(stack_memory);
    thread->kernel_stack_top = thread->kernel_stack_bottom + thread->stack_size;

    // Initialize scheduling fields
    thread->priority = DEFAULT_PRIORITY;
    thread->time_slice_remaining = DEFAULT_TIME_SLICE;
    thread->total_runtime = 0;

    // Copy name
    usize len = strlen(name);
    if (len >= sizeof(thread->name)) len = sizeof(thread->name) - 1;
    memcpy(thread->name, name, len);
    thread->name[len] = '\0';

    // Setup initial stack frame
    setup_thread_stack(thread, entry_point);

    // Add thread to process
    ProcessManager::add_thread(process, thread);

    drivers::serial_printf("[Thread] Created thread %d: %s (stack: 0x%lx-0x%lx)\n",
                          thread->tid, name,
                          thread->kernel_stack_bottom,
                          thread->kernel_stack_top);

    return thread;
}

Thread* ThreadManager::get_current() {
    return current_thread_;
}

void ThreadManager::set_current(Thread* thread) {
    current_thread_ = thread;
}

[[noreturn]] void ThreadManager::exit_thread(int exit_code) {
    drivers::serial_printf("[Thread] Thread %d exiting with code %d\n",
                          current_thread_->tid, exit_code);

    current_thread_->state = ThreadState::TERMINATED;

    // Remove from scheduler
    Scheduler::remove_thread(current_thread_);

    // Yield to next thread (never returns)
    Scheduler::yield();

    // Should never reach here
    while (true) {
        asm volatile("cli; hlt");
    }
}

void ThreadManager::yield() {
    Scheduler::yield();
}

uint32 ThreadManager::allocate_tid() {
    return next_tid_++;
}

void ThreadManager::setup_thread_stack(Thread* thread, void (*entry_point)()) {
    // Stack grows downward, so start from top
    uint64* stack = reinterpret_cast<uint64*>(thread->kernel_stack_top);

    // Build initial stack frame for context_switch
    // The stack layout matches what context_switch expects:

    // Push dummy interrupt frame (for kernel threads)
    --stack; *stack = 0x10;                          // ss (kernel data segment)
    --stack; *stack = thread->kernel_stack_top - 8;  // rsp
    --stack; *stack = 0x202;                         // rflags (IF=1, reserved bit=1)
    --stack; *stack = 0x08;                          // cs (kernel code segment)
    --stack; *stack = reinterpret_cast<uint64>(thread_entry);  // rip

    // Push general purpose registers (all initialized to 0)
    --stack; *stack = 0;  // rax
    --stack; *stack = 0;  // rbx
    --stack; *stack = 0;  // rcx
    --stack; *stack = 0;  // rdx
    --stack; *stack = 0;  // rsi
    --stack; *stack = 0;  // rdi
    --stack; *stack = 0;  // rbp
    --stack; *stack = 0;  // r8
    --stack; *stack = 0;  // r9
    --stack; *stack = 0;  // r10
    --stack; *stack = 0;  // r11
    --stack; *stack = 0;  // r12
    --stack; *stack = 0;  // r13
    --stack; *stack = 0;  // r14
    --stack; *stack = 0;  // r15

    // Push entry point address (will be popped by thread_entry)
    --stack; *stack = reinterpret_cast<uint64>(entry_point);

    // Save stack pointer
    thread->cpu_state = reinterpret_cast<CpuState*>(stack);

    drivers::serial_printf("[Thread] Stack setup: cpu_state at 0x%lx\n",
                          reinterpret_cast<uint64>(thread->cpu_state));
}

} // namespace tiny_os::process

// C wrapper for thread exit (called from assembly)
extern "C" void thread_exit_wrapper() {
    tiny_os::process::ThreadManager::exit_thread(0);
}
