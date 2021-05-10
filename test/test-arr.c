#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "coro.h"


#define UNUSED __attribute__((unused))
#define N_CORO 2560
#define BASE 16
#define FLUCTUATE 32


void work(void){
    int n_loop = (int)coro_get_arg();
    size_t counter = 0;
    char UNUSED _holder[256];

    for (int i = 0; i < n_loop; ++i){
        counter += i;
    }

    coro_return();
}

coro_t *coro_arr[N_CORO], *main_co;

int main(void){
    coro_stack_t *sstack;

    sstack = coro_stack_new(0, 1);
    main_co = coro_new_main();

    printf("running %d coroutines\n", N_CORO);

    int loops;
    for (int i = 0; i < N_CORO; ++i){
        loops = BASE + rand()%FLUCTUATE;
        coro_arr[i] = coro_new(
            work, (void*)loops, main_co, 130, sstack
        );
    }

    coro_t *cur_co;
    for (int i = 0; i < N_CORO; ++i){
        cur_co = coro_arr[i];
        coro_resume(cur_co);
        assert(cur_co->is_ended != 0);
    }

    for (int i = 0; i < N_CORO; ++i){
        coro_free(coro_arr[i]);
        coro_arr[i] = NULL;
    }
    printf(
        "max stack usage: %lu, base addr: %p\n",
        sstack->stat.max_stack_usage, sstack->raw_ptr
    );
    coro_free(main_co); main_co = NULL;
    coro_stack_free(sstack); sstack = NULL;


    return EXIT_SUCCESS;
}