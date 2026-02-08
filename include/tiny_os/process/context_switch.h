#pragma once

#include <tiny_os/process/thread.h>

namespace tiny_os::process {

// Context switch assembly functions
extern "C" {
    // Switch from old_thread to new_thread
    // This saves the current CPU state to old_thread->cpu_state
    // and restores CPU state from new_thread->cpu_state
    void context_switch(CpuState** old_state, CpuState* new_state);

    // First-time thread entry point wrapper
    // This is called when a thread runs for the first time
    void thread_entry();
}

} // namespace tiny_os::process
