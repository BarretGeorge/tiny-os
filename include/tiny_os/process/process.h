#pragma once

#include <tiny_os/common/types.h>
#include <tiny_os/memory/page_table.h>

namespace tiny_os::process {

// Forward declarations
class Thread;

// Process states
enum class ProcessState {
    CREATED,        // Just created, not yet scheduled
    RUNNING,        // Currently executing
    READY,          // Ready to run
    BLOCKED,        // Waiting for resource
    ZOMBIE,         // Terminated but not yet cleaned up
    TERMINATED      // Fully terminated
};

const char* process_state_to_string(ProcessState state);

// Process Control Block (PCB)
struct Process {
    uint32 pid;                             // Process ID
    ProcessState state;                     // Current state
    memory::PageTable* page_table;          // Process address space (nullptr for kernel process)

    // Threads
    Thread* main_thread;                    // Main thread
    Thread** threads;                       // Array of all threads
    usize thread_count;                     // Number of threads
    usize max_threads;                      // Max threads capacity

    // Parent/child relationships
    Process* parent;                        // Parent process
    Process** children;                     // Array of child processes
    usize child_count;                      // Number of children
    usize max_children;                     // Max children capacity

    // Exit status
    int exit_code;                          // Exit code (for zombie processes)

    // Name (for debugging)
    char name[64];
};

// Process Manager
class ProcessManager {
public:
    // Initialize the process manager
    static void init();

    // Create a kernel process (runs in kernel mode)
    static Process* create_kernel_process(const char* name, void (*entry_point)());

    // Get current process
    static Process* get_current();

    // Find process by PID
    static Process* find_process(uint32 pid);

    // Terminate a process
    static void terminate_process(uint32 pid, int exit_code);

    // Add a thread to a process
    static void add_thread(Process* process, Thread* thread);

    // Remove a thread from a process
    static void remove_thread(Process* process, Thread* thread);

    // Print process list (for debugging)
    static void print_process_list();

private:
    static constexpr usize MAX_PROCESSES = 256;
    static constexpr usize INITIAL_THREADS_PER_PROCESS = 4;
    static constexpr usize INITIAL_CHILDREN_PER_PROCESS = 4;

    static Process* processes_[MAX_PROCESSES];
    static uint32 next_pid_;
    static Process* current_process_;

    static uint32 allocate_pid();
};

} // namespace tiny_os::process
