#pragma once

#include <tiny_os/common/types.h>
#include <tiny_os/process/process.h>

namespace tiny_os::process {

// Thread states
enum class ThreadState {
    CREATED,        // Just created
    READY,          // Ready to run
    RUNNING,        // Currently executing
    BLOCKED,        // Waiting for resource
    TERMINATED      // Finished execution
};

const char* thread_state_to_string(ThreadState state);

// CPU state saved during context switch
struct CpuState {
    // General purpose registers (pushed by context_switch)
    uint64 r15, r14, r13, r12, r11, r10, r9, r8;
    uint64 rbp, rdi, rsi, rdx, rcx, rbx, rax;

    // For user-mode threads (pushed by interrupt)
    uint64 rip;             // Instruction pointer
    uint64 cs;              // Code segment
    uint64 rflags;          // CPU flags
    uint64 rsp;             // Stack pointer
    uint64 ss;              // Stack segment
} __attribute__((packed));

// Thread Control Block (TCB)
struct Thread {
    uint32 tid;                         // Thread ID
    Process* process;                   // Parent process
    ThreadState state;                  // Current state

    // CPU state (saved/restored during context switch)
    CpuState* cpu_state;                // Pointer to saved state on stack

    // Stack information
    VirtualAddress kernel_stack_top;    // Top of kernel stack
    VirtualAddress kernel_stack_bottom; // Bottom of kernel stack
    usize stack_size;                   // Stack size

    // Scheduling
    int priority;                       // Priority (0-31, higher = more important)
    uint64 time_slice_remaining;        // Remaining time slice (ticks)
    uint64 total_runtime;               // Total runtime (ticks)

    // Name (for debugging)
    char name[64];
};

// Thread Manager
class ThreadManager {
public:
    // Initialize the thread manager
    static void init();

    // Create a kernel thread
    static Thread* create_kernel_thread(Process* process, const char* name,
                                       void (*entry_point)());

    // Get current thread
    static Thread* get_current();

    // Set current thread
    static void set_current(Thread* thread);

    // Terminate current thread
    [[noreturn]] static void exit_thread(int exit_code);

    // Sleep current thread (yield)
    static void yield();

private:
    static constexpr usize DEFAULT_STACK_SIZE = 16 * 1024;  // 16KB
    static constexpr int DEFAULT_PRIORITY = 10;
    static constexpr uint64 DEFAULT_TIME_SLICE = 10;        // 10 ticks = 100ms @ 100Hz

    static uint32 next_tid_;
    static Thread* current_thread_;

    static uint32 allocate_tid();

    // Setup initial stack frame for new thread
    static void setup_thread_stack(Thread* thread, void (*entry_point)());
};

} // namespace tiny_os::process
