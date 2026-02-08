#pragma once

#include <tiny_os/common/types.h>
#include <tiny_os/process/thread.h>

namespace tiny_os::process {

// Simple Round-Robin scheduler
class Scheduler {
public:
    // Initialize the scheduler
    static void init();

    // Start scheduling (enables timer-based preemption)
    static void start();

    // Add a thread to the ready queue
    static void add_thread(Thread* thread);

    // Remove a thread from the ready queue
    static void remove_thread(Thread* thread);

    // Schedule next thread (called from timer interrupt)
    static void schedule();

    // Yield CPU to next thread
    static void yield();

    // Block current thread (remove from ready queue)
    static void block_current();

    // Unblock a thread (add to ready queue)
    static void unblock_thread(Thread* thread);

    // Get current thread
    static Thread* current_thread();

    // Print scheduler statistics
    static void print_stats();

private:
    static constexpr usize MAX_READY_THREADS = 256;

    // Simple circular queue for round-robin scheduling
    static Thread* ready_queue_[MAX_READY_THREADS];
    static usize ready_queue_head_;
    static usize ready_queue_tail_;
    static usize ready_queue_size_;

    static Thread* current_thread_;
    static Thread* idle_thread_;
    static bool scheduling_enabled_;

    static uint64 context_switches_;
    static uint64 idle_time_;

    // Get next thread from ready queue
    static Thread* get_next_thread();

    // Perform context switch
    static void switch_to(Thread* next_thread);
};

} // namespace tiny_os::process
