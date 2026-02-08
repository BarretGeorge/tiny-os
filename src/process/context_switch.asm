[bits 64]

; Context switch between threads
; void context_switch(CpuState** old_state, CpuState* new_state)
; rdi = pointer to old thread's cpu_state pointer
; rsi = new thread's cpu_state value
global context_switch
context_switch:
    ; Save old thread's context
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; For kernel threads, we need to save additional state
    ; Push dummy values for interrupt frame (rip, cs, rflags, rsp, ss)
    pushfq                  ; Save rflags
    mov rax, cs
    push rax                ; Save cs
    lea rax, [rel .return]  ; Save return address as rip
    push rax
    push rsp                ; Save current rsp (will be adjusted)
    mov rax, ss
    push rax                ; Save ss

    ; Save current stack pointer to *old_state
    mov [rdi], rsp

    ; Load new thread's stack pointer
    mov rsp, rsi

    ; Restore new thread's context
    ; Pop the interrupt frame
    pop rax                 ; ss (ignore)
    pop rax                 ; rsp (ignore, will use current)
    pop rax                 ; rip (will return to this address)
    mov rbx, rax            ; Save return address
    pop rax                 ; cs (ignore)
    popfq                   ; Restore rflags

    ; Restore general purpose registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Jump to saved rip
    push rbx
    ret

.return:
    ; Old thread returns here after being scheduled again
    ret

; Thread entry point wrapper
; This is where new threads start execution
global thread_entry
thread_entry:
    ; At this point, the stack has been set up by setup_thread_stack
    ; The actual thread function address is on the stack

    ; Enable interrupts (new thread should allow preemption)
    sti

    ; Pop the thread function address and call it
    pop rax                 ; Get entry_point address
    call rax                ; Call the thread function

    ; If thread function returns, exit the thread
    ; This calls thread_exit (will be implemented in C++)
    extern thread_exit_wrapper
    call thread_exit_wrapper

    ; Should never reach here
    cli
    hlt
