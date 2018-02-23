#include "thread.h"
#include "interrupt.h"
#include <ucontext.h>
#include <stdlib.h>

struct thread_t {
	ucontext_t* 	context;
	char*			stack;
	bool			finished;
}

static thread_t* active_thread;

// fifo queues
static queue<thread_t> ready;
static queue<thread_t> blocked;

static bool libinit_completed = false;

int thread_libinit(thread_startfunc_t func, void *arg) {
	interrupt_disable();

	if( libinit_completed ) {
		interrupt_enable();
		return 1; // Unsuccessful
	}

	// create new thread. 
	thread_create(func, arg);


	libinit_completed = true;

	interrupt_enable();
	return 0; 
}

int thread_create(thread_startfunc_t func, void *arg) {

	this_thread = (thread_t*) malloc(sizeof(thread_t));

	/* ---------------- Create a new context ---------------- */

	if (!getcontext(newthread->context)) {
		// error handling
	}

	ucontext_t* ucontext_ptr = newthread;

	char *stack = new char [STACK_SIZE];
	ucontext_ptr->uc_stack.ss_sp = stack;
	ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
	ucontext_ptr->uc_stack.ss_flags = 0;
	ucontext_ptr->uc_link = NULL;

	makecontext(ucontext_ptr, func, 1, arg);

	interrupt_enable();
	func(arg);
	interrupt_disable();

	// destroy thread, update state of thread lib. 
}

int thread_yield(void) {
	// save current context => move to end of ready.
	// pop head from ready => set to active
	// new set context
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