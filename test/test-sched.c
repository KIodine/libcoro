#include <stdio.h>
#include <stdlib.h>

#include "coro.h"
#include "list.h"

#define N_CORO 256


typedef struct {
    coro_t* coro;
    struct list list;
} task_t;

/*
typedef struct sched_s sched_t;
struct sched_s {
    void *ctx;
    void (*enqueue)(sched_t*, coro_t*);
    coro_t *(*dequeue)(sched_t*);
} sched_t;

sched_t *coro_sched_new(int policy);
void coro_sched_free(sched_t *sched);

cur_coro = sched->dequeue(sched);
sched->enqueue(sched, coro);

*/


static
void work(void)
{
    int n = (int)coro_get_arg();
    int count = 0;

    for (int i = 0; i < 12; ++i){
        count += i;
        coro_yield();
    }

    coro_return();
}

int main(void)
{
    coro_stack_t *sstack = NULL;
    coro_t *main_co = NULL;
    list_decl(head);

    coro_thread_env_save();

    sstack = coro_stack_new_guarded(0);
    main_co = coro_new_main();

    task_t *tmp_tsk;
    for (int i = 0; i < N_CORO; ++i){
        tmp_tsk = calloc(1, sizeof(task_t));
        tmp_tsk->coro = coro_new(work, (void*)i, main_co, 128, sstack);
        list_push(&head, &tmp_tsk->list);
    }

    coro_t *cur_co;
    struct list *cur_node;
    for (;!list_is_empty(&head);){
        cur_node = list_get(&head);
        tmp_tsk = list_entry(cur_node, task_t, list);
        cur_co = tmp_tsk->coro;

        coro_resume(cur_co);
        if (cur_co->is_ended != 0){
            // destroy task.
            coro_free(tmp_tsk->coro);
            free(tmp_tsk);
        } else {
            // re-schedule.
            list_push(&head, &tmp_tsk->list);
        }
    }

    coro_free(main_co); main_co = NULL;
    coro_stack_free(sstack); sstack = NULL;

    return EXIT_SUCCESS;
}