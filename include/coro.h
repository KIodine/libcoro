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
    int is_ended;
};


/* The variable for thread local currently running coroutine. */
/* TODO:
- Custom check macros.
- Wrap access to global variable into `coro_*` function.
- Rearrange struct layout.
- Add shortcuts for certain operations, such as allocating a main coro.
- (PROPOSAL)Add statistic fields to structs.
- Add more checks, assertions.
- Test abort mechanism and test stack overflow.
- (STUDY) The possibility of return from triggered raw-return protection.
*/

extern __thread coro_t *coro_tls_co;
extern __thread coro_fp_t coro_tls_ret_warn;

#define coro_set_ret_warn(fp) ((void)0, coro_tls_ret_warn = (fp))

//void coro_ret_warn(void); /* may call default warning or `coro_tls_ret_warn`. */
extern void coro_stack_ret(void) __asm__("coro_stack_ret");

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
#define coro_yield() do { \
    coro_switch(coro_tls_co, coro_tls_co->from_co);\
} while (0)

#define coro_return() do { \
    coro_tls_co->is_ended = 1;\
    coro_tls_co->stack->last_owner = NULL;\
    coro_yield();\
} while (0)

/* How to prevent this being a left value? */
#define coro_get_co() ((void)0, (coro_tls_co))
#define coro_get_arg() ((void)0, (coro_tls_co->arg))

#define coro_is_main(co) ((co)->from_co == NULL)


#endif /* CORO_H */