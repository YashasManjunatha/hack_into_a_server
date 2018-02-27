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

void loop2(void *a) {
  char *id;
  int i;

  id = (char *) a;
  cout << "loop called with id " << (char *) id << endl;

  if (thread_yield()) {
    cout << "thread_yield failed\n";
    exit(1);
  }

  for (i=0; i<5; i++, g++) {
    cout << id << ":\t" << i << "\t" << g << endl;
    if (thread_yield()) {
      cout << "thread_yield failed\n";
      exit(1);
    }
  }
}

/* ----------------------------- CASE 5 TEST ----------------------------- */
// Same as case 4, but calls loop2, which has a yield before the for-loop
// that prints the global integer and local integer successively
void case5(void *a) {
  int arg;
  arg = (long int) a;

  cout << "case5 called with arg " << arg << endl;
  int i = 0;
  while( i < 7 ){
  	if (thread_create(loop2, (void *) "child thread")) {
	    cout << "thread_create failed\n";
	    exit(1);
	  }
	  i++;
  }

  loop2( (void *) "parent thread");
}

/* ----------------------------- MAIN FUNCTION ----------------------------- */
int main(int argc, char *argv[]) {
	if (thread_libinit(case5, (void *) 5)) {
		cout << "thread_libinit failed\n";
		exit(1);
	}
  	exit(0);
}