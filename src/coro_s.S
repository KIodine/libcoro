.intel_syntax noprefix
.text

.global coro_switch
# param: rdi: coro_t*, rsi: coro_t*
# scratch: rax, rcx
coro_switch:
    mov rax, QWORD PTR [rsp]        # old rip
    lea rcx, [rsp+0x8]              # old rsp
    mov QWORD PTR [rdi+0x0], rbx
    mov QWORD PTR [rdi+0x8], rcx    # rsp
    mov QWORD PTR [rdi+0x10], rbp
    mov QWORD PTR [rdi+0x18], rax   # rip
    mov QWORD PTR [rdi+0x20], r12
    mov QWORD PTR [rdi+0x28], r13
    mov QWORD PTR [rdi+0x30], r14
    mov QWORD PTR [rdi+0x38], r15
    fnstcw WORD PTR [rdi+0x40]
    stmxcsr DWORD PTR [rdi+0x44]
    mov rbx, QWORD PTR [rsi+0x0]
    mov rcx, QWORD PTR [rsi+0x8]    # rsp
    mov rbp, QWORD PTR [rsi+0x10]
    mov rax, QWORD PTR [rsi+0x18]   # rip
    mov r12, QWORD PTR [rsi+0x20]
    mov r13, QWORD PTR [rsi+0x28]
    mov r14, QWORD PTR [rsi+0x30]
    mov r15, QWORD PTR [rsi+0x38]
    fldcw WORD PTR [rsi+0x40]
    ldmxcsr DWORD PTR [rsi+0x44]
    mov rsp, rcx
    jmp rax

.extern coro_ret_warn
.extern abort
.global coro_stack_ret
coro_stack_ret:
    and rsp, 0xfffffffffffffff0    # force align 16-byte boundary
    call coro_ret_warn
    call abort
