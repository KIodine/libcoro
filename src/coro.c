#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>

#include "coro.h"


//static void default_ret_warn(void);

__thread coro_t *tls_co = NULL;
__thread coro_fp_t tls_ret_warn = NULL;


// static default_ret_warn(void){...}

void coro_ret_warn(void)
{
    /* FIXME: decide how to handle use of raw return in coroutine. */
    printf("Current coroutine: %p\n", tls_co);
    /* call `tls_ret_warn` or `default_ret_warn` */
    return;
}

coro_stack_t *coro_stack_new(size_t sz_hint, int enable_guard_page)
{
    int res = 0;
    long page_sz = 0;
    size_t stack_sz = 0;
    void *stack_addr = NULL;
    coro_stack_t *sstack = NULL;

    /* make sz the multiple of page. */
    page_sz = sysconf(_SC_PAGESIZE);
    /* ensure page size if power of 2 */
    assert(((page_sz - 1) & page_sz) == 0);
    /* ensure page size is 16-byte aligned and is valid value. */
    assert((page_sz & 0xf) == 0 && page_sz > 0);

    if (sz_hint == 0){
        /* default size: 4MB = 2^(2+10+10) */
        stack_sz = 1 << 22;
    } else if (sz_hint < (size_t)page_sz){
        stack_sz = (size_t)page_sz;
    }
    if (enable_guard_page != 0){
        /* Add an additional page for stack overflow protection. */
        stack_sz += (size_t)page_sz;
    }

    sstack = calloc(1, sizeof(coro_stack_t));
    sstack->is_guard_page_enabled = enable_guard_page;

    stack_addr = mmap(
        NULL, stack_sz, PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS, -1, 0
    );
    assert(MAP_FAILED != stack_addr);
    /* ensure addr is 16-byte aligned. */
    assert(((uintptr_t)stack_addr & 0xf) == 0);
    if (enable_guard_page){
        /* Deny any operation on this section. */
        res = mprotect(stack_addr, page_sz, PROT_NONE);
        assert(res == 0);
    }
    sstack->raw_ptr = stack_addr;
    sstack->raw_sz  = stack_sz;
    
    void *aligned_ptr;
    /* move ptr forward, as if it is allocated that way. */
    stack_addr += (ptrdiff_t)page_sz;
    stack_sz   -= (size_t)page_sz;

    aligned_ptr = (void*)((uintptr_t)stack_addr & (~0xf));
    if (((uintptr_t)stack_addr & 0xf) != 0){
        /* Unconditionally "carry". */
        aligned_ptr += 0x10;
    }

    /* `aligned_top` is 16-byte_aligned. */
    sstack->align_top = (void*)(
        (((uintptr_t)stack_addr + stack_sz) & ~(0xf)) - sizeof(uintptr_t)*2
    );
    /*  `align_limit` includes pointer `aligned_top`, range:
        [`aligned_ptr`, `aligned_top`], 16-byte aligned. */
    sstack->align_limit  = sstack->align_top - aligned_ptr + sizeof(uintptr_t)*2;
    sstack->ret_addr_ptr = sstack->align_top + sizeof(uintptr_t);
    /*  put a function at the bottom of stack, a "normal" return will
        trigger this function. */
    *(void**)(sstack->ret_addr_ptr) = (void*)coro_stack_ret;

    return sstack;
}

void coro_stack_free(coro_stack_t *sstack){
    munmap(sstack->raw_ptr, sstack->raw_sz);
    free(sstack);
    return;
}

coro_t *coro_new(
    coro_fp_t fp, void *arg,
    coro_t *from_co, size_t sz_hint,
    coro_stack_t *sstack
){
    coro_t *co = NULL;

    co = calloc(1, sizeof(coro_t));
    co->arg = arg;
    co->func = fp;
    if (from_co != NULL){
        co->stack = sstack;
        co->from_co = from_co;
        /* setup necessary registers. */
        co->reg[REG_IDX_IP] = (void*)fp;
        co->reg[REG_IDX_SP] = sstack->ret_addr_ptr;
        if (sz_hint == 0){
            sz_hint = 128;
        }
        co->mem.raw_ptr = malloc(sz_hint);
        co->mem.raw_sz  = sz_hint;
        memset(co->mem.raw_ptr, 0, sz_hint);
    }
    
    return co;
}

void coro_free(coro_t *co)
{
    if (co->from_co != NULL){
        if (co->stack->last_owner == co){
            co->stack->last_owner = NULL;
        }
        free(co->mem.raw_ptr);
    }
    free(co);
    return;
}

void coro_reset(coro_t *co)
{
    memset(co->reg, 0, sizeof(co->reg));
    co->reg[REG_IDX_IP] = (void*)co->func;
    co->reg[REG_IDX_SP] = co->stack->ret_addr_ptr;
    co->is_ended = 0;
    co->mem.valid_sz = 0;

    return;
}

void coro_resume(coro_t *co)
{
    size_t stack_use = 0;
    coro_stack_t *sstack = co->stack;
    coro_mem_t *mem = NULL;
    coro_t *own_co = NULL;

    /* PROPOSAL: Die if try to resume main co or an ended co? */
    
    if (sstack->last_owner == co)
    {
        /* no need to swap stack content, skip saving process. */
        goto do_switch;
    }
    /* do preserve to private mem. */
    if (sstack->last_owner != NULL)
    {
        own_co  = sstack->last_owner;
        mem     = &(own_co->mem);

        /* TODO: update max stack usage on saving coro. */

        /* Include `sp`, but not `ret_addr_ptr` */
        stack_use = sstack->ret_addr_ptr - own_co->reg[REG_IDX_SP];
        // check stack overflow
        assert(
            sstack->align_top - (sstack->align_limit - sizeof(uintptr_t)*2)
            <=
            own_co->reg[REG_IDX_SP]
        );
        /*  Stack usage may vary according to the depth of calls of 
            the last owner. */
        if (own_co->mem.raw_sz < stack_use){
            for(;;){
                /*  double the size of private stack until it is suitable
                    for storage. */
                mem->raw_sz <<= 1;
                if (mem->raw_sz >= stack_use){
                    break;
                }
            }
            free(mem->raw_ptr);
            mem->raw_ptr = malloc(mem->raw_sz);
            memset(mem->raw_ptr, 0, mem->raw_sz);
        }
        memcpy(mem->raw_ptr, own_co->reg[REG_IDX_SP], stack_use);
        /* update size of new content. */
        own_co->mem.valid_sz = stack_use;
        // update `n_saved`
        sstack->last_owner = NULL;
    }
    /* restore stuffs from private mem to shared stack. */
    if (co->mem.valid_sz > 0){
        /* if it is not the first time coroutine resumes. */
        void *cpy_base = (void*)(
            (uintptr_t)sstack->ret_addr_ptr - (uintptr_t)co->mem.valid_sz
        );
        memcpy(cpy_base, co->mem.raw_ptr, co->mem.valid_sz);
        // update `n_restored`
    }
    sstack->last_owner = co;

do_switch:
    // update `n_resumed`?
    tls_co = co;
    coro_switch(co->from_co, co);
    tls_co = co->from_co;
    
    /*  calculate max stack usage here, so we can record the stack usage of
        a one-time coroutine. */
    stack_use = sstack->ret_addr_ptr - co->reg[REG_IDX_SP];
    if (stack_use > sstack->stat.max_stack_usage){
        sstack->stat.max_stack_usage = stack_use;
    }

    return;
}
