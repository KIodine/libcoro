#ifndef CORO_H
#define CORO_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

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
        size_t n_resumed;
    } stat;
} coro_mem_t;

typedef struct {
    void *raw_ptr; /* The base address of allocated space. */
    size_t raw_sz; /* The size of allocated memory, including protected page, if such presents. */
    void *align_top; /* Address of stack "bottom", 16-byte aligned. */
    size_t valid_sz; /* The size of stack being used. */
    size_t align_limit; /* The maximum size the stack is allowed to use. */
    void *ret_addr_ptr; /* the address of IP on the stack. */
    coro_t *last_owner; /* last coroutine using the shared stack. */
    /* --- flags --- */
    int is_guard_page_enabled;
} coro_stack_t;

struct coro_s {
    void *reg[9];
    coro_stack_t *stack;
    coro_mem_t mem;
    coro_fp_t func;
    void *arg;
    coro_t *from_co;
    /* --- flags --- */
    int is_ended;
};


/* The variable for thread local currently running coroutine. */
/*
*/
extern __thread coro_t *tls_co;
extern __thread coro_fp_t tls_ret_warn;
void coro_ret_warn(void); /* may call default warning or `tls ret_warn`. */
extern void coro_stack_ret(void) __asm__("coro_stack_ret");

coro_stack_t *coro_stack_new(size_t sz_hint, int enable_guard_page);
void coro_stack_free(coro_stack_t *stack);
coro_t *coro_new(
    coro_fp_t fp, void *arg,
    coro_t *from_co, size_t sz_hint,
    coro_stack_t *sstack
);
void coro_free(coro_t *co);

extern void coro_switch(coro_t *from, coro_t *to) __asm__("coro_switch");

void coro_resume(coro_t *co);
void coro_reset(coro_t *co);

#define coro_yield() do { \
    coro_switch(tls_co, tls_co->from_co);\
} while (0)

#define coro_return() do { \
    tls_co->is_ended = 1;\
    tls_co->stack->last_owner = NULL;\
    coro_yield();\
} while (0)

/*
void coro_yield(void);
void coro_return(void);
*/

#endif /* CORO_H */