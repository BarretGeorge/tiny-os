#include <tiny_os/process/process.h>
#include <tiny_os/process/thread.h>
#include <tiny_os/memory/heap_allocator.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>
#include <tiny_os/common/string.h>

namespace tiny_os::process {

// Static member definitions
Process* ProcessManager::processes_[MAX_PROCESSES];
uint32 ProcessManager::next_pid_ = 1;
Process* ProcessManager::current_process_ = nullptr;

const char* process_state_to_string(ProcessState state) {
    switch (state) {
        case ProcessState::CREATED: return "CREATED";
        case ProcessState::RUNNING: return "RUNNING";
        case ProcessState::READY: return "READY";
        case ProcessState::BLOCKED: return "BLOCKED";
        case ProcessState::ZOMBIE: return "ZOMBIE";
        case ProcessState::TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

void ProcessManager::init() {
    drivers::serial_printf("[Process] Initializing process manager...\n");

    // Clear process table
    for (usize i = 0; i < MAX_PROCESSES; i++) {
        processes_[i] = nullptr;
    }

    drivers::serial_printf("[Process] Process manager initialized\n");
    drivers::kprintf("[Process] Process manager initialized\n");
}

Process* ProcessManager::create_kernel_process(const char* name, void (*entry_point)()) {
    drivers::serial_printf("[Process] Creating kernel process: %s\n", name);

    // Allocate PCB
    Process* process = new Process();
    if (!process) {
        drivers::serial_printf("[Process] Failed to allocate PCB!\n");
        return nullptr;
    }

    // Initialize PCB
    process->pid = allocate_pid();
    process->state = ProcessState::CREATED;
    process->page_table = nullptr;  // Kernel processes use kernel page table

    // Initialize thread list
    process->threads = new Thread*[INITIAL_THREADS_PER_PROCESS];
    process->thread_count = 0;
    process->max_threads = INITIAL_THREADS_PER_PROCESS;

    // Initialize children list
    process->children = new Process*[INITIAL_CHILDREN_PER_PROCESS];
    process->child_count = 0;
    process->max_children = INITIAL_CHILDREN_PER_PROCESS;

    process->parent = nullptr;
    process->exit_code = 0;

    // Copy name
    usize len = strlen(name);
    if (len >= sizeof(process->name)) len = sizeof(process->name) - 1;
    memcpy(process->name, name, len);
    process->name[len] = '\0';

    // Create main thread
    process->main_thread = ThreadManager::create_kernel_thread(process, name, entry_point);
    if (!process->main_thread) {
        drivers::serial_printf("[Process] Failed to create main thread!\n");
        delete[] process->threads;
        delete[] process->children;
        delete process;
        return nullptr;
    }

    // Add to process table
    if (process->pid < MAX_PROCESSES) {
        processes_[process->pid] = process;
    }

    drivers::serial_printf("[Process] Created process %d: %s\n", process->pid, name);

    return process;
}

Process* ProcessManager::get_current() {
    return current_process_;
}

Process* ProcessManager::find_process(uint32 pid) {
    if (pid < MAX_PROCESSES) {
        return processes_[pid];
    }
    return nullptr;
}

void ProcessManager::terminate_process(uint32 pid, int exit_code) {
    Process* process = find_process(pid);
    if (!process) return;

    drivers::serial_printf("[Process] Terminating process %d with exit code %d\n",
                          pid, exit_code);

    process->state = ProcessState::ZOMBIE;
    process->exit_code = exit_code;

    // TODO: Clean up threads, memory, etc.
    // For now, just mark as zombie
}

void ProcessManager::add_thread(Process* process, Thread* thread) {
    if (!process || !thread) return;

    // Check if we need to expand the array
    if (process->thread_count >= process->max_threads) {
        usize new_max = process->max_threads * 2;
        Thread** new_threads = new Thread*[new_max];

        // Copy existing threads
        for (usize i = 0; i < process->thread_count; i++) {
            new_threads[i] = process->threads[i];
        }

        delete[] process->threads;
        process->threads = new_threads;
        process->max_threads = new_max;
    }

    // Add thread
    process->threads[process->thread_count++] = thread;
}

void ProcessManager::remove_thread(Process* process, Thread* thread) {
    if (!process || !thread) return;

    // Find and remove thread
    for (usize i = 0; i < process->thread_count; i++) {
        if (process->threads[i] == thread) {
            // Shift remaining threads
            for (usize j = i; j < process->thread_count - 1; j++) {
                process->threads[j] = process->threads[j + 1];
            }
            process->thread_count--;
            break;
        }
    }
}

void ProcessManager::print_process_list() {
    drivers::kprintf("\n=== Process List ===\n");
    drivers::kprintf("PID  State      Threads  Name\n");
    drivers::kprintf("---  ---------  -------  ----\n");

    for (usize i = 0; i < MAX_PROCESSES; i++) {
        Process* proc = processes_[i];
        if (proc) {
            drivers::kprintf("%3d  %-9s  %7d  %s\n",
                           proc->pid,
                           process_state_to_string(proc->state),
                           proc->thread_count,
                           proc->name);
        }
    }

    drivers::kprintf("\n");
}

uint32 ProcessManager::allocate_pid() {
    return next_pid_++;
}

} // namespace tiny_os::process
