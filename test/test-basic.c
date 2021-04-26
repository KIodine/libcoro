#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "coro.h"


#define DEBUG __attribute__((unused))

static void work2(void)
{
    int DEBUG arr[4096];
    for (int i = 0; i < 4096; ++i){
        arr[i] = rand();
    }
    //return;
    coro_return();
}

static void work(void)
{
    int *arg = coro_get_co()->arg;
    char DEBUG _unused[1024] = {0};
    for (int i = 0; i < 6; i++){
        //printf("i = %d\n", i);
        *arg += 1;
        coro_yield();
    }
    coro_return();
}


int main(void)
{
    int count = 0;
    coro_stack_t *sstack;
    coro_t *main_co, *co_a, *co_b;
    
    sstack = coro_stack_new(0, 1);
    
    main_co = coro_new_main();
    co_a = coro_new(work, (void*)&count, main_co, 128, sstack);
    co_b = coro_new(work, (void*)&count, main_co, 16, sstack);

    coro_t *co_tmp = coro_new(work2, NULL, main_co, 100, sstack);
    coro_resume(co_tmp);
    coro_free(co_tmp);

    //assert(tls_co == NULL);
    /*
    */
    printf("pre-run a: %p\n", (void*)co_a);
    coro_resume(co_a);
    printf("pre-run b: %p\n", (void*)co_b);
    coro_resume(co_b);
    
    printf("run co_a\n");
    for (;;){
        coro_resume(co_a);
        if (co_a->is_ended != 0){
            break;
        }
        printf("count: %d\n", count);
    }
    assert(tls_co == main_co);
    printf("run co_b\n");
    for (;;){
        coro_resume(co_b);
        if (co_b->is_ended != 0){
            break;
        }
        printf("count: %d\n", count);
    }
    assert(tls_co == main_co);
    coro_reset(co_a);
    printf("run reset a\n");
    for (;;){
        coro_resume(co_a);
        if (co_a->is_ended != 0){
            break;
        }
        printf("count: %d\n", count);
    }
    assert(tls_co == main_co);

    printf("max stack use: %lu\n", sstack->stat.max_stack_usage);

    coro_free(co_a); co_a = NULL;
    coro_free(co_b); co_b = NULL;
    coro_free(main_co); main_co = NULL;

    coro_stack_free(sstack); sstack = NULL;
    
    return EXIT_SUCCESS;
}