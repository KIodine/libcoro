#include <stdio.h>
#include <stdlib.h>

#include "coro.h"


static void work(void){
    int *arg = (int*)tls_co->arg;
    for (int i = 0; i < 6; i++){
        *arg = i;
        coro_yield();
    }
    coro_return();
}

int main(void)
{
    int count = 0;
    coro_stack_t *sstack;
    coro_t *main_co, *co_0;
    
    sstack = coro_stack_new(0, 1);
    
    main_co = coro_new(NULL, NULL, NULL, 0, NULL);
    co_0 = coro_new(work, (void*)&count, main_co, 128, sstack);

    for (;;){
        coro_resume(co_0);
        if (co_0->is_ended != 0){
            break;
        }
        printf("count: %d\n", count);
    }


    coro_free(co_0); co_0 = NULL;
    coro_free(main_co); main_co = NULL;

    coro_stack_free(sstack); sstack = NULL;
    
    return EXIT_SUCCESS;
}