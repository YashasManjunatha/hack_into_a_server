#include "thread.h"
#include "interrupt.h"
#include <ucontext.h>
#include <stdlib.h>

struct thread_t {
	ucontext_t* 	context;
	char*			stack;
	bool			done;
}

static thread_t* active_thread;

// fifo queues
static queue<thread_t> ready;
static queue<thread_t> blocked;

static bool libinit_completed = false;

extern int thread_libinit(thread_startfunc_t func, void *arg) {
	interrupt_disable();

	if( libinit_completed ) {
		interrupt_enable();
		return 1; // tries to reinitialize libinit
	} else {
		libinit_completed = true;
	}

	// create new thread. 
	interrupt_enable();
	thread_create(func, arg);
	interrupt_disable();

	active_thread = ready.pop_front();

	setcontext(active_thread->context); // do we set or do we swap context? 

	// while there are threads to run, run them....
	while(!ready.empty()) {
		if (active_thread->done) {
			delete_thread(active_thread);
		}

	}
	// Clean up all our memmory. 

	cout << "Thread library exiting.\n";
	interrupt_enable();
	return 0; 
}

extern int thread_create(thread_startfunc_t func, void *arg) {
	interrupt_disable();

	if ( !libinit_completed ) {
		interrupt_enable();
		return 1; // error: 1 - libinit not called successfully
	}

	thread_t new_thread = new thread_t;
	new_thread->context = new ucontext_t;
	new_thread->stack 	= new char[STACK_SIZE];
	new_thread->done	= false;

	/* ---------------- Create a new context ---------------- */

	if (!getcontext(new_thread->context)) {
		interrupt_enable();
		return 1;
	}

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
}

extern int thread_yield(void) {
	// save current context => move to end of ready.
	// pop head from ready => set to active
	// new set context
	interrupt_disable();
	ucontext_t prevContext = active_thread->context;
	ready.push_back( active_thread ); 
	active_thread = ready.pop_front();
	// TODO: Check to see if we should be using swap_context here because it will auto-run
	// NOTE: if using swap_context here, enable line below
	// interrupt_enable();
	swap_context( prevContext, active_thread->context );
	// interrupt_disable();
	interrupt_enable();
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