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
	thread_t* 			holderID;
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
int getcontext_ec(ucontext_t*);
int swapcontext_ec(ucontext_t*, ucontext_t*);
int delete_thread(thread_t* t);
int run_stub(thread_startfunc_t, void*);

int thread_create_helper(thread_startfunc_t func, void* arg);
int thread_yield_helper(void);
int thread_lock_helper(unsigned int lock);
int thread_unlock_helper(unsigned int lock);
int thread_wait_helper(unsigned int lock, unsigned int cond);
int thread_signal_helper(unsigned int lock, unsigned int cond);
int thread_broadcast_helper(unsigned int lock, unsigned int cond);

/* -------------------------------------------------------------------------- */
/* 					MAJOR INTERRUPT ASSUMPTIONS FOR ALL FILE				  */
/* -------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
	MAJOR INTERRUPT ASSUMPTIONS FOR ALL FILES
	1) All thread files (e.g. libinit, yield, create, etc.) START assuming that
		interrupt is enabled, and END with interrupt enabled as well.
	2) swap_context assumes that it STARTS with interrupts disabled, and it
		ENDS with interrupts disabled as well
	3) handle_error STARTS with interrupt disabled, and ENDS with interrupt
		enabled!
 ---------------------------------------------------------------------------- */

// Assumptions: STARTS interrupt_disable, ENDS interrupt_enable
/* int handle_error( void *msg ){
    do { cout << (char *) msg << endl; interrupt_enable(); return -1; } while (0);
} */

int handle_error( const std::string& msg ){
    // do { cout << (char *) msg << endl; return -1; } while (0);
    cout << msg;
    return -1;
}

// Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_libinit(thread_startfunc_t func, void* arg) {
	assert_interrupts_enabled();
	interrupt_disable();

	if( libinit_completed ) {
		// interrupt_enable(); // Taken care of in handle_error
		return handle_error( "libinit already called; can't call libinit again." );
	} else {
		libinit_completed = true;
	}

	if (thread_create_helper(func, arg) == -1) {
		return -1;
	}

	manager_context = new ucontext_t;

	if (getcontext_ec(manager_context) == -1) {
		return -1;
	}

	// while there are threads to run, run them....
	while(!ready.empty()) {
		active_thread = ready.front();
		ready.pop();

		if (active_thread->done) {
			if (delete_thread(active_thread) == -1) {
				return -1;
			}
		} else {
			// I'm PRETTY certain there should be enable interrupts here... double check
			// interrupt_disable(); // TODO: WARNING, CHECK THIS IMMEDIATELY. *****
			if (swapcontext_ec(manager_context, active_thread->context) == -1) {
				return -1;
			}
		} // will this delete active threads that weren't on the ready queue? ==> push back in the run stub.
	}

	// Clean up all our memory.
	// for everything in blocked, clean it out.

	cout << "Thread library exiting.\n";
	assert_interrupts_disabled();
	interrupt_enable();
	exit(EXIT_SUCCESS);
}

// Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_create(thread_startfunc_t func, void* arg) {
	assert_interrupts_enabled();
	interrupt_disable();

	int success = thread_create_helper( func, arg );

	assert_interrupts_disabled();
	interrupt_enable();
	return success;
}

// Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_yield(void) {
	assert_interrupts_enabled();
	interrupt_disable();

	int success = thread_yield_helper();

	assert_interrupts_disabled();
	interrupt_enable();
	return success;
}

/*///////////////////////  ^^ FINISH THESE FIRST ^^ ////////////////////////*/

// Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_lock(unsigned int lock) {
	assert_interrupts_enabled();
	interrupt_disable();

	int success = thread_lock_helper( lock );

	assert_interrupts_disabled();
	interrupt_enable();
	return success;
}

// Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_unlock(unsigned int lock) {
	assert_interrupts_enabled();
	interrupt_disable();

	int success = thread_unlock_helper( lock );

	assert_interrupts_disabled();
	interrupt_enable();
	return success;
}

// Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_wait(unsigned int lock, unsigned int cond) {
	assert_interrupts_enabled();
	interrupt_disable();

	int success = thread_wait_helper( lock, cond );

	assert_interrupts_disabled();
	interrupt_enable();
	return success;
}

// Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_signal(unsigned int lock, unsigned int cond) {
	assert_interrupts_enabled();
	interrupt_disable();

	int success = thread_signal_helper( lock, cond );

	assert_interrupts_disabled();
	interrupt_enable();
	return success;
}

// Assumptions: STARTS interrupt_enable, ENDS interrupt_enable
int thread_broadcast(unsigned int lock, unsigned int cond) {
	assert_interrupts_enabled();
	interrupt_disable();

	int success = thread_broadcast_helper( lock, cond );

	assert_interrupts_disabled();
	interrupt_enable();
	return success;
}

/* ---------------- HELPER THREAD FX ---------------- */

int thread_create_helper(thread_startfunc_t func, void* arg) {
	try{
		if ( !libinit_completed ) {
			return handle_error( "libinit hasn't been called before creating thread" );
		}

		thread_t* new_thread = new thread_t;
		new_thread->context = new ucontext_t;
		new_thread->stack 	= new char[STACK_SIZE];
		new_thread->done	= false;

		/* ---------------- Create a new context ---------------- */

		if (getcontext_ec(new_thread->context) == -1) {
			return -1;
		}

		/* -------------------Configure context ------------------------ */
		new_thread->context->uc_stack.ss_sp 	= new_thread->stack;
		new_thread->context->uc_stack.ss_size 	= STACK_SIZE;
		new_thread->context->uc_stack.ss_flags 	= 0;
		new_thread->context->uc_link 			= NULL;

		/* ---------------- Deliver function to context ---------------- */
		makecontext(new_thread->context, (void (*)()) run_stub, 2, func, arg);

		ready.push(new_thread);
	} catch(int e) {
		return -1;
	}
	
	return 0;
}

int thread_yield_helper(void) {
	try{
		if ( !libinit_completed ) {
			return handle_error( "libinit hasn't been called before yielding thread" );
		}

		ready.push( active_thread );
		if (swapcontext_ec(active_thread->context, manager_context) == -1) {
			return -1; // flag: swapcontext to manager
		}
	} catch( int e ){
		return -1;
	}

	return 0;
}

int thread_lock_helper(unsigned int lock) {
	try{
		if ( !libinit_completed ) {
			return handle_error( "libinit hasn't been called before locking thread!" );
		}

		map<int,lock_t*>::iterator it = lock_map.find(lock);
		if (it == lock_map.end()) {
			// not found
			lock_t* new_lock = new lock_t;
			new_lock->id = lock;
			new_lock->held = true;
			new_lock->holderID = active_thread;
			lock_map[lock] = new_lock;
		} else {
			// found
			lock_t* old_lock = it->second;

			if (old_lock->held) {
				// Throw error if holder already holds this lock
				if( old_lock->holderID == active_thread ) {
					return handle_error( "thread has tried to acquire a lock it already holds!" );
				}
				old_lock->waiting.push(active_thread);
				if (swapcontext_ec(active_thread->context, manager_context) == -1){
					return -1;
				}
			} else {
				// gives lock to the active Thread
				old_lock->held = true;
				old_lock->holderID = active_thread;
			}

		}
	} catch( int e ){
		return -1;
	}

	return 0;
}

int thread_unlock_helper(unsigned int lock) {
	try{
		if ( !libinit_completed ) {
			return handle_error( "libinit hasn't been called before unlocking thread" );
		}

		map<int,lock_t*>::iterator it = lock_map.find(lock);
		if (it == lock_map.end()) {
			// lock not found! what?
			// TODO: Possibly throw an error. We need to keep track of such things
			return handle_error( "thread_unlock has tried to find lock, but couldn't!" );
		} else {
			lock_t* old_lock = it->second;

			// Throw error if thread tries to unlock lock it doesn't own!
			if( old_lock->holderID != active_thread ){
				return handle_error( "thread_unlock has tried to unlock lock that it doesn't own!" );
			}

			if (old_lock->waiting.empty()) {
				old_lock->held = false;
			} else {
				// pops off something from waiting and puts it into ready queue
				// old_lock is still held by the new ready queue thread
				ready.push(old_lock->waiting.front());
				old_lock->holderID = old_lock->waiting.front();
				old_lock->waiting.pop();
			}
		}
	} catch( int e ){
		return -1;
	}

	return 0;
}

int thread_wait_helper(unsigned int lock, unsigned int cond) {
	try{
		if ( !libinit_completed ) {
			//interrupt_enable(); // Taken care of in handle_error
			return handle_error( "libinit hasn't been called before waiting for thread cv" );
		}

		if (thread_unlock_helper(lock) == -1) {
			return -1;
		}

		map<int,cv_t*>::iterator it = cv_map.find(cond);

		if (it == cv_map.end()) {
			// not found
			cv_t* new_cv = new cv_t;
			new_cv->id = cond;
			new_cv->waiting.push(active_thread);
			cv_map[cond] = new_cv;
			if (swapcontext_ec(active_thread->context, manager_context) == -1) {
				return -1; // flag: swapcontext to manager
			}
		} else {
			// found
			cv_t* old_cv = it->second;
			old_cv->waiting.push(active_thread);
			if (swapcontext_ec(active_thread->context, manager_context) == -1) {
				return -1; // flag: swapcontext to manager
			}
		}

		if (thread_lock_helper(lock) == -1) {
			return -1;
		}
		// interrupt_enable(); // lock ends up with interrupts enabled, so this is redundant
	} catch( int e ){
		return -1;
	}
	
	return 0;
}

int thread_signal_helper(unsigned int lock, unsigned int cond) {
	try{
		if ( !libinit_completed ) {
			//interrupt_enable(); // Taken care of in handle_error
			return handle_error( "libinit hasn't been called before signalling thread" );
		}

		// If the CV waiter queue is not empty, a thread wakes up: it moves from the head of
		// the CV waiter queue to the tail of the ready queue (blocked -> ready).

		map<int,cv_t*>::iterator it = cv_map.find(cond);

		if (it == cv_map.end()) {
			// Is okay that CV is not found when signalling.
		} else {
			cv_t* old_cv = it->second;
			if (!old_cv->waiting.empty()) {
				ready.push(old_cv->waiting.front());
				old_cv->waiting.pop();
			}
		}
	} catch( int e ){
		return -1;
	}
	
	return 0;
}

int thread_broadcast_helper(unsigned int lock, unsigned int cond) {
	try{
		if ( !libinit_completed ) {
			//interrupt_enable(); // Taken care of in handle_error
			return handle_error( "libinit hasn't been called before broadcasting to threads." );
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
	} catch( int e ){
		return -1;
	}

	return 0;
}

/* ---------------- HELPER FUNCTIONS ---------------- */

int getcontext_ec(ucontext_t* a) { // error checking
	if (getcontext(a) != 0) {
		//interrupt_enable(); // Taken care of in handle_error
		return handle_error( "call to getcontext failed." );
	}
	return 0;
}

// Assumptions: STARTS interrupt_disable, ENDS interrupt_disable
int swapcontext_ec(ucontext_t* a, ucontext_t* b) { // error checking
	// Are we sure we shouldn't be enabling interrupts becacuse context is swapping
	// and therefore the function SHOULD run with interrupts?
	if (swapcontext(a, b) == -1) {
		return handle_error( "call to swap_context failed." );
	}
	return 0;
}

// Assumptions: START interrupt_disable, ENDS interrupt_disable
// NOTE: DO NOT NEED TO INTERRUPT_DISABLE because it does shit
int delete_thread(thread_t* t) {
	try{
		char* stack = t->stack;
		delete(t->context);
		delete(t);
		delete(stack);
	} catch( int e ){
		return -1;
	}
	
	return 0;
}

int run_stub(thread_startfunc_t func, void *arg) {
	try{
		assert_interrupts_disabled(); interrupt_enable();
		func(arg);
		assert_interrupts_enabled(); interrupt_disable();

		active_thread->done = true;
		ready.push(active_thread);
		swapcontext(active_thread->context, manager_context); // flag: swapcontext to manager
	} catch( int e ){
		return -1;
	}

	return 0;
} 