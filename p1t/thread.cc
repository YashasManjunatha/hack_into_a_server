#include "thread.h"
#include "interrupt.h"
#include <ucontext.h>
#include <stdlib.h>

// useful macro borrowed from: https://linux.die.net/man/3/swapcontext
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct thread_t {
	ucontext_t* 	context;
	char*			stack;
	bool			done;
}

static thread_t* 	active_thread;
static ucontext_t* 	manager_context;

// fifo queues
static queue<thread_t> ready;
static queue<thread_t> blocked;

static bool libinit_completed = false;

extern int thread_libinit(thread_startfunc_t func, void *arg) {
	interrupt_disable();

	if( libinit_completed ) {
		interrupt_enable();
		handle_error("libinit already called");
	} else {
		libinit_completed = true;
	}

	interrupt_enable();
	thread_create(func, arg);
	interrupt_disable();

	getcontext_ec(manager_context);

	// while there are threads to run, run them....
	while(!ready.empty()) {
		active_thread = ready.pop_front();

		if (active_thread->done) {
			delete_thread(active_thread);
		} else {
			swapcontext_ec(manager_context, active_thread->context);
		}
	}

	// Clean up all our memmory. 

	cout << "Thread library exiting.\n";
	interrupt_enable();
	exit(EXIT_SUCCESS);
}

extern int thread_create(thread_startfunc_t func, void *arg) {
	interrupt_disable();

	if ( !libinit_completed ) {
		interrupt_enable();
		handle_error("libinit hasn't been called before creating thread");
	}

	thread_t new_thread = new thread_t;
	new_thread->context = new ucontext_t;
	new_thread->stack 	= new char[STACK_SIZE];
	new_thread->done	= false;

	/* ---------------- Create a new context ---------------- */

	getcontext_ec(new_thread->context);

	/* -------------------Configure context ------------------------ */
	new_thread->context->uc_stack.ss_sp 	= new_thread->stack;
	new_thread->context->uc_stack.ss_size 	= STACK_SIZE;
	new_thread->context->uc_stack.ss_flags 	= 0;
	new_thread->context->uc_link 			= NULL;

	/* ---------------- Deliver function to context ---------------- */
	makecontext(new_thread->context, run_stub, 2, func, arg);

	ready.push_back(new_thread);
	interrupt_enable();
	return 0;
}

extern int thread_yield(void) {
	interrupt_disable();

	ready.push_back( active_thread ); 
	swap_context_ec(active_thread->context, manager_context);

	interrupt_enable();
	return 0;
}

/*///////////////////////  ^^ FINISH THESE FIRST ^^ ////////////////////////*/

extern int thread_lock(unsigned int lock) {

}

extern int thread_unlock(unsigned int lock) {

}

extern int thread_wait(unsigned int lock, unsigned int cond) {

}

extern int thread_signal(unsigned int lock, unsigned int cond) {

}

extern int thread_broadcast(unsigned int lock, unsigned int cond) {

}

/* ---------------- HELPER FUNCTIONS ---------------- */

void getcontext_ec(ucontext_t* a) { // error checking
	if (!getcontext(a)) {
		interrupt_enable();
		handle_error("call to getcontext failed.");
	}
}

void swap_context_ec(ucontext_t* a, ucontext_t* b) { // error checking
	if (swap_context(a, b) == -1) {
		interrupt_enable();
		handle_error("call to swap_context failed.");
	}
}

int delete_thread(thread_t t) {
	char* stack = t->stack;
	delete(t->context);
	delete(t);
	delete(stack);
}

int run_stub(thread_startfunc_t func, void *arg) {
	interrupt_enable();
	func(arg);
	interrupt_disable();

	active_thread->done = true;
	swap_context(active_thread->context, manager_context);
}