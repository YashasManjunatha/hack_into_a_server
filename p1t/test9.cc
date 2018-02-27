#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>

using namespace std;

/* ----------------------------- CASE 9 TEST ----------------------------- */
// wait without holding a lock. 

void case1(void *a) {
  int arg;
  arg = (long int) a;
  if (thread_wait(1, 1) == -1) {
  	exit(1);
  }
}

/* ----------------------------- MAIN FUNCTION ----------------------------- */
int main(int argc, char *argv[]) {
	if (thread_libinit(case1, (void *) 1)) {
    	cout << "thread_libinit failed\n";
    	exit(1);
  	}
  	exit(0);
}