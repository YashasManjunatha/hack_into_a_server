#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>

using namespace std;

int g=0;

void loop(void *a) {
	  if(thread_yield()) {
		cout << "thread_yield failed\n";
      	exit(1);
	}


	if (thread_lock(1)) {
		cout << "thread lock failed\n";
		exit(1);
	}

	if(thread_yield()) {
		cout << "thread_yield failed\n";
      	exit(1);
	}
	char *id;
  int i;

  id = (char *) a;
  cout << "loop called with id " << (char *) id << endl;


	for (int i = 0; i < 5; i++, g++) {
		 cout << id << ":\t" << i << "\t" << g << endl;
		if(thread_yield()) {
		cout << "thread_yield failed\n";
      	exit(1);
      }
	}

	if (g != 5) {
		cout << "test failed\n";
		exit(1);
	} else {
    cout << "SUCCESS\n";
  }

	if (thread_unlock(2) != -1) {
  		cout << "thread unlock on not previously existing lock\n";
  		exit(1);
  }

  if(thread_yield()) {
		cout << "thread_yield failed\n";
      	exit(1);
	}


  if (thread_unlock(1)) {
  	cout << "thread_unlock fails\n";
  	exit(1);
  }

	  if(thread_yield()) {
		cout << "thread_yield failed\n";
      	exit(1);
	}
}

void launch(void *a) {
  int arg;
  arg = (long int) a;

  cout << "case1 called with arg " << arg << endl;

  if(thread_yield()) {
		cout << "thread_yield failed\n";
      	exit(1);
	}

  	if (thread_create(loop, (void *) "child thread")) {
  		cout << "thread_create failed\n";
   		exit(1);
  	}
}


int main(int argc, char *argv[]) {
  if (thread_libinit(launch, (void *) 3)) {
    cout << "thread_libinit failed\n";
    exit(1);
  }
}