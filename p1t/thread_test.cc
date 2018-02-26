// To compile: g++ -o thread_test thread.cc thread_test.cc libinterrupt.a -ldl

#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>

using namespace std;

int g=0;

void loop(void *a) {
  char *id;
  int i;

  id = (char *) a;
  cout << "loop called with id " << (char *) id << endl;

  for (i=0; i<5; i++, g++) {
    cout << id << ":\t" << i << "\t" << g << endl;
    if (thread_yield()) {
      cout << "thread_yield failed\n";
      exit(1);
    }
  }
}

/* ----------------------------- CASE 1 TEST ----------------------------- */
// Tests to see if thread_create works and if thread will yield to itself
// If it is the only thing in in the active and ready queues
void case1(void *a) {
  int arg;
  arg = (long int) a;

  cout << "case1 called with arg " << arg << endl;
  if (thread_create(loop, (void *) "child thread")) {
    cout << "thread_create failed\n";
    exit(1);
  }
}

/* ----------------------------- CASE 2 TEST ----------------------------- */
// Same as case1, except tests to see if the parent thread can loop after
// thread_create's child thread
void case2(void *a) {
  int arg;
  arg = (long int) a;

  cout << "case2 called with arg " << arg << endl;
  if (thread_create(loop, (void *) "child thread")) {
    cout << "thread_create failed\n";
    exit(1);
  }

  loop( (void *) "parent thread");
}

/* ----------------------------- CASE 3 TEST ----------------------------- */
// Creates numLoops number of threads that run the loop function
void case3(void *a) {
  int arg;
  arg = (long int) a;

  cout << "case3 called with arg " << arg << endl;
  int i = 0;
  while( i < 7 ){
  	if (thread_create(loop, (void *) "child thread")) {
	    cout << "thread_create failed\n";
	    exit(1);
	  }
	  i++;
  }
}

/* ----------------------------- CASE 4 TEST ----------------------------- */
// Creates numLoops number of threads that run the loop function with
// parent function calling loop as well
void case4(void *a) {
  int arg;
  arg = (long int) a;

  cout << "case3 called with arg " << arg << endl;
  int i = 0;
  while( i < 7 ){
  	if (thread_create(loop, (void *) "child thread")) {
	    cout << "thread_create failed\n";
	    exit(1);
	  }
	  i++;
  }

  loop( (void *) "parent thread");
}

/* ----------------------------- MAIN FUNCTION ----------------------------- */
int main(int argc, char *argv[]) {
  if( argc < 2 ){
  	cout << "Input must select at least one test\n";
  	exit(1);
  }
  int testChoice = atoi(argv[1]);
  switch( testChoice ){
  	case 1:
  		if (thread_libinit(case1, (void *) 1)) {
		    cout << "thread_libinit failed\n";
		    exit(1);
		  }
		  break;
	  case 2:
	  	if (thread_libinit(case2, (void *) 2)) {
		    cout << "thread_libinit failed\n";
		    exit(1);
		  }
	  	break;
	  case 3:
	  	if (thread_libinit(case3, (void *) 3)) {
		    cout << "thread_libinit failed\n";
		    exit(1);
		  }
	  	break;
	  case 4:
	  	if (thread_libinit(case4, (void *) 4)) {
		    cout << "thread_libinit failed\n";
		    exit(1);
		  }
	  	break;
  }
  exit(0);
}