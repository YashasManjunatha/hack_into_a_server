#include "thread.h"
#include <ucontext.h>


// Notes

// /*
// * Initialize a context structure by copying the current thread's context.
// */
// getcontext(ucontext_ptr); // ucontext_ptr has type (ucontext_t *)

// /*
// * Direct the new thread to use a different stack. Your thread library
// * should allocate STACK_SIZE bytes for each thread's stack.
// */
// char *stack = new char [STACK_SIZE];
// ucontext_ptr->uc_stack.ss_sp = stack;
// ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
// ucontext_ptr->uc_stack.ss_flags = 0;
// ucontext_ptr->uc_link = NULL;

// /*
// * Direct the new thread to start by calling start(arg1, arg2).
// */
// makecontext(ucontext_ptr, (void (*)()) start, 2, arg1, arg2);

int thread_libinit(thread_startfunc_t func, void *arg) {

}

int thread_create(thread_startfunc_t func, void *arg) {

}

int thread_yield(void) {

}


/*///////////////////////  ^^ FINISH THESE FIRST ^^ ////////////////////////*/

int thread_lock(unsigned int lock) {

}

int thread_unlock(unsigned int lock) {

}

int thread_wait(unsigned int lock, unsigned int cond) {

}

int thread_signal(unsigned int lock, unsigned int cond) {

}

int thread_broadcast(unsigned int lock, unsigned int cond) {

}