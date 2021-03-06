#ifndef CORO_H
#define CORO_H

#include <stddef.h>
#include <stdint.h>

#if defined (__x86_64__)
#else
    #error "platform not supported"
#endif


enum {
    REG_IDX_SP = 1,
    REG_IDX_BP = 2,
    REG_IDX_IP = 3,
    REG_IDX_FPMX = 8,   // lower half: fpu, higher half: mxcsr.
};


typedef struct coro_s coro_t;

typedef void (*coro_fp_t)(void);


typedef struct {
    void *raw_ptr; /* The base address of allocated memory. */
    size_t raw_sz; /* The size of allocated memory. */
    size_t valid_sz; /* The real size of stack that is utilized by coroutine. */
    struct {
        /* Anonymous struct for storing statistical data. */
        size_t max_mem_usage;
    } stat;
} coro_mem_t;


typedef struct {
    void *raw_ptr; /* The base address of allocated space. */
    size_t raw_sz; /* The size of allocated memory, including protected page, if such presents. */
    void *align_top; /* Address of stack "bottom", 16-byte aligned. */
    //size_t valid_sz; /* The size of stack being used. */
    size_t align_limit; /* The maximum size the stack is allowed to use. */
    void *ret_addr_ptr; /* the address of IP on the stack. */
    coro_t *last_owner; /* last coroutine using the shared stack. */
    struct {
        size_t max_stack_usage;
    } stat;
    /* --- flags --- */
    int is_guard_page_enabled;
} coro_stack_t;


struct coro_s {
    void *reg[9];   /* !do not move this member, it is used by assembly code! */
    coro_stack_t *stack;
    coro_mem_t mem;
    coro_fp_t func;
    void *arg;
    coro_t *from_co;
    /* --- flags --- */
    int is_ended;   // bit-flag instead?
};


/* The variable for thread local currently running coroutine. */
/* TODO:
- Custom check macros.
- Add more checks, assertions.
- Test abort mechanism and test stack overflow.
- (STUDY) The possibility of return from triggered raw-return protection.
    After triggered the protection, SP is a `sizeof(void*)` above stack and BP
    being 0 or other value depends on the initial value set in `coro_new`;
    programmers MUST avoid the usage of stack(local variable storage) inside
    protection routine. The only way of breaking out is calling (presumed)
    `coro_recover`(it just loads the enviroment of main coro without preserve
    the current env) and mark coroutine ended/exception, or utilizing `setjmp/longjmp`.
*/

extern __thread coro_t *coro_tls_co;
extern __thread coro_fp_t coro_tls_ret_warn;

void coro_thread_init(coro_fp_t ret_cb);
#define coro_thread_env_save() coro_thread_init(NULL)


coro_stack_t *coro_stack_new(size_t sz_hint, int enable_guard_page);
void coro_stack_free(coro_stack_t *stack);
coro_t *coro_new(
    coro_fp_t fp, void *arg,
    coro_t *from_co, size_t sz_hint,
    coro_stack_t *sstack
);
void coro_free(coro_t *co);

#define coro_stack_new_guarded(sz_hint) coro_stack_new((sz_hint), 1)
#define coro_new_main() coro_new(NULL, NULL, NULL, 0, NULL)

extern void coro_switch(coro_t *from, coro_t *to) __asm__("coro_switch");

void coro_resume(coro_t *co);
void coro_reset(coro_t *co);

/* ensure pointer is not NULL? */
/* TODO: forbid yield to same shared stack. */
#define coro_yield() do { \
    coro_switch(coro_tls_co, coro_tls_co->from_co);\
} while (0)

#define coro_return() do { \
    coro_tls_co->is_ended = 1;\
    coro_tls_co->stack->last_owner = NULL;\
    coro_yield();\
} while (0)

/* prevent this being a left value? */
#define coro_get_co() ((void)0, (coro_tls_co))
#define coro_get_arg() ((void)0, (coro_tls_co->arg))

#define coro_is_main(co) ((co)->from_co == NULL)


#endif /* CORO_H */