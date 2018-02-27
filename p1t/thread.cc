#include "thread.h"
#include "interrupt.h"
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <queue>
#include <map>
using namespace std;

// useful macro borrowed from: https://linux.die.net/man/3/swapcontext

struct thread_t {
	ucontext_t* 	context;
	char*			stack;
	bool			done;
};

struct lock_t {
	int 				id;
	bool 				held;
	queue<thread_t*>	waiting;
};

struct cv_t {
	int 				id;
	queue<thread_t*>	waiting;
};

static thread_t* 	active_thread;
static ucontext_t* 	manager_context;
// fifo queues
static queue<thread_t*> ready;
static queue<thread_t*> blocked;

static map<int, lock_t*> 	lock_map;
static map<int, cv_t*> 		cv_map;

static bool libinit_completed = false;

/* ---------------------- FUNCTION STUB DECLARATIONS ---------------------- */
void getcontext_ec(ucontext_t*);
void swapcontext_ec(ucontext_t*, ucontext_t*);
int delete_thread(thread_t* t);
int run_stub(thread_startfunc_t, void*);

//	Assumptions: STARTS interrupt_disable, ENDS interrupt_enable
int handle_error( void *msg ){
    do { cout << (char *) msg << endl; interrupt_enable(); return -1; } while (0);
}

//	Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_libinit(thread_startfunc_t func, void* arg) {
	interrupt_disable();

	if( libinit_completed ) {
		// interrupt_enable(); // Taken care of in handle_error
		return handle_error( (void*) "libinit already called; can't call libinit again." );
	} else {
		libinit_completed = true;
	}

	interrupt_enable();
	thread_create(func, arg);
	interrupt_disable();

	manager_context = new ucontext_t;

	getcontext_ec(manager_context);

	// while there are threads to run, run them....
	while(!ready.empty()) {
		active_thread = ready.front();
		ready.pop();

		if (active_thread->done) {
			delete_thread(active_thread);
		} else {
			// I'm PRETTY certain there should be enable interrupts here... double check
			// interrupt_disable(); // TODO: WARNING, CHECK THIS IMMEDIATELY. *****
			swapcontext_ec(manager_context, active_thread->context);
		} // will this delete active threads that weren't on the ready queue? ==> push back in the run stub.
	}

	// Clean up all our memory.
	// for everything in blocked, clean it out.

	cout << "Thread library exiting.\n";
	interrupt_enable();
	exit(EXIT_SUCCESS);
}

//	Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_create(thread_startfunc_t func, void* arg) {
	interrupt_disable();

	if ( !libinit_completed ) {
		//interrupt_enable();
		return handle_error( (void*) "libinit hasn't been called before creating thread" );
	}

	thread_t* new_thread = new thread_t;
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
	makecontext(new_thread->context, (void (*)()) run_stub, 2, func, arg);

	ready.push(new_thread);
	interrupt_enable();
	return 0;
}

//	Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_yield(void) {
	interrupt_disable();

	if ( !libinit_completed ) {
		//interrupt_enable();
		return handle_error( (void*) "libinit hasn't been called before yielding thread" );
	}

	ready.push( active_thread );
	swapcontext_ec(active_thread->context, manager_context); // flag: swapcontext to manager

	interrupt_enable();
	return 0;
}

/*///////////////////////  ^^ FINISH THESE FIRST ^^ ////////////////////////*/

//	Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_lock(unsigned int lock) {
	interrupt_disable();

	if ( !libinit_completed ) {
		//interrupt_enable();
		return handle_error( (void*) "libinit hasn't been called before locking thread" );
	}

	map<int,lock_t*>::iterator it = lock_map.find(lock);
	if (it == lock_map.end()) {
		// not found
		lock_t* new_lock = new lock_t;
		new_lock->id = lock;
		new_lock->held = true;
		lock_map[lock] = new_lock;
	} else {
		// found
		lock_t* old_lock = it->second;

		if (old_lock->held) {
			old_lock->waiting.push(active_thread);
			swapcontext_ec(active_thread->context, manager_context);
		} else {
			old_lock->held = true;
		}

	}

	interrupt_enable();
	return 0;
}

//	Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_unlock(unsigned int lock) {
	interrupt_disable();

	if ( !libinit_completed ) {
		//interrupt_enable();
		return handle_error( (void*) "libinit hasn't been called before unlocking thread" );
	}

	map<int,lock_t*>::iterator it = lock_map.find(lock);
	if (it == lock_map.end()) {
		// lock not found! what?
		// TODO: Possibly throw an error. We need to keep track of such things
	} else {
		lock_t* old_lock = it->second;
		if (old_lock->waiting.empty()) {
			old_lock->held = false;
		} else {
			// pops off something from waiting and puts it into ready queue
			// old_lock is still held by the new ready queue thread
			ready.push(old_lock->waiting.front());
			old_lock->waiting.pop();
		}
	}

	interrupt_enable();
	return 0;
}

//	Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_wait(unsigned int lock, unsigned int cond) {
	interrupt_disable();

	if ( !libinit_completed ) {
		//interrupt_enable();
		return handle_error( (void*) "libinit hasn't been called before waiting for thread cv" );
	}

	interrupt_enable();
	thread_unlock(lock);
	interrupt_disable();

	map<int,cv_t*>::iterator it = cv_map.find(cond);

	if (it == cv_map.end()) {
		// not found
		cv_t* new_cv = new cv_t;
		new_cv->id = cond;
		new_cv->waiting.push(active_thread);
		cv_map[cond] = new_cv;
		swapcontext_ec(active_thread->context, manager_context); // flag: swapcontext to manager
	} else {
		// found
		cv_t* old_cv = it->second;
		old_cv->waiting.push(active_thread);
		swapcontext_ec(active_thread->context, manager_context); // flag: swapcontext to manager
	}

	// TODO: Double check the logic of this existing
	interrupt_enable();
	thread_lock(lock);
	// interrupt_enable(); // lock ends up with interrupts enabled, so this is redundant
	return 0;
}

//	Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_signal(unsigned int lock, unsigned int cond) {
	interrupt_disable();

	if ( !libinit_completed ) {
		//interrupt_enable();
		return handle_error( (void*) "libinit hasn't been called before signalling thread" );
	}

	// If the CV waiter queue is not empty, a thread wakes up: it moves from the head of
	// the CV waiter queue to the tail of the ready queue (blocked -> ready).

	map<int,cv_t*>::iterator it = cv_map.find(cond);

	if (it == cv_map.end()) {
		// cv not found! what?
		// TODO: Throw error
	} else {
		cv_t* old_cv = it->second;
		if (!old_cv->waiting.empty()) {
			ready.push(old_cv->waiting.front());
			old_cv->waiting.pop();
		}
	}

	interrupt_enable();
	return 0;
}

//	Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_broadcast(unsigned int lock, unsigned int cond) {
	interrupt_disable();

	if ( !libinit_completed ) {
		//interrupt_enable();
		return handle_error( (void*) "libinit hasn't been called before broadcasting to threads." );
	}

	// If the CV waiter queue is not empty, a thread wakes up: it moves from the head of
	// the CV waiter queue to the tail of the ready queue (blocked -> ready).

	map<int,cv_t*>::iterator it = cv_map.find(cond);

	if (it == cv_map.end()) {
		// cv not found! what?
		// TODO: Throw error
	} else {
		cv_t* old_cv = it->second;
		while (!old_cv->waiting.empty()) {
			ready.push(old_cv->waiting.front());
			old_cv->waiting.pop();
		}
	}

	interrupt_enable();
	return 0;
}

/* ---------------- HELPER FUNCTIONS ---------------- */

void getcontext_ec(ucontext_t* a) { // error checking
	if (getcontext(a) != 0) {
		//interrupt_enable();
		handle_error( (void*) "call to getcontext failed." );
	}
}

//	Assumptions: STARTS interrupt_disable, ENDS interrupt_disable
void swapcontext_ec(ucontext_t* a, ucontext_t* b) { // error checking
	// Are we sure we shouldn't be enabling interrupts becacuse context is swapping
	// and therefore the function SHOULD run with interrupts?
	interrupt_enable();
	if (swapcontext(a, b) == -1) {
		interrupt_disable();
		handle_error( (void*) "call to swap_context failed." );
	} else {
		interrupt_disable();
	}
}

int delete_thread(thread_t* t) {
	char* stack = t->stack;
	delete(t->context);
	delete(t);
	delete(stack);
	return 0;
}

int run_stub(thread_startfunc_t func, void *arg) {
	//interrupt_enable();
	func(arg);
	//interrupt_disable();

	active_thread->done = true;
	ready.push(active_thread);
	swapcontext(active_thread->context, manager_context); // flag: swapcontext to manager
	return 0;
} 