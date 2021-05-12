# libcoro
`libcoro` is a library implements the idea of a coroutine using shared stack.
This project is a trimmed version of [`libaco`](https://github.com/hnes/libaco)(great project!), is build for study only, use with cautious!

## Reuiqrement
This library is highly machine dependent, currently supported platform:
- x86_64 SysV ABI

## about coroutine
The idea of coroutine is simple, for most of programs there are several things to make it running properly: ABI and stack.
- ABI(Abstract Binary Interface) constraints how parameters and local varaibles should preserve and passed between caller and callee.
- Stack stores local variables used by callee/function.

Note that because of lazy stack preservation, yielding from coroutine does not preserve the current stack (that would make it more complicate).

To use "nested" corutine, you must use another shared stack for that coroutine, otherwise resuming from the nested one will clobber the stack of current coroutine.

## Build
```bash
make static # or `shared` for building shared version.
```
## Quickstart
Threadlocal enviroment initialize:
```c
void coro_thread_init(coro_fp_t ret_cb);
// if no callback is set, use macro:
coro_thread_env_save()
```

Shared stack operations:
```c
coro_stack_t *coro_stack_new(size_t sz_hint, int enable_guard_page);
void coro_stack_free(coro_stack_t *stack);
// macro for allocating a stack with guarded page:
coro_stack_new_guarded(sz_hint)
```

Coroutine operations:
```c
coro_t *coro_new(
    coro_fp_t fp, void *arg,
    coro_t *from_co, size_t sz_hint,
    coro_stack_t *sstack
);
void coro_free(coro_t *co);

// Switch execution enviroment to specific coroutine.
void coro_resume(coro_t *co);
// Reset coroutine to initial state.
void coro_reset(coro_t *co);

// Retrive the pointer to current running co. Is `NULL` before first resume.
coro_t *coro_get_co(void)
// Shorthand for getting argument of current coroutine.
void* coro_get_arg(void)

// Macros for control flow inside coroutine:
// resume to main coroutine of current coroutine.
coro_yield()
// Return from current coroutine and mark current coroutine as ended.
// you must perform return inside coroutine with this macro, otherwise
// the instruction flow will be busted.
coro_return()
```

## License
`libcoro` is distributed under MIT license.
