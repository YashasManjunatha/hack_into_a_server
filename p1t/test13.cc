#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>

using namespace std;

int g=0;

void loop(void *a) {
  thread_yield();
  thread_lock(1);
  thread_yield();
	char *id;
  int i;

  id = (char *) a;
  cout << "loop called with id " << (char *) id << endl;

  g++;

  thread_signal(2,2);
  thread_yield();

	if (g != 2) {
		cout << "test failed\n";
		exit(1);
	} else {
    cout << "SUCCESS\n";
  }
  thread_unlock(1);
}

void launch(void *a) {
  thread_yield();
  thread_lock(2);
  thread_yield();
  int arg;
  arg = (long int) a;

  cout << "case1 called with arg " << arg << endl;

  	if (thread_create(loop, (void *) "child thread")) {
  		cout << "thread_create failed\n";
   		exit(1);
  	}

    thread_wait(2, 2);

    if (g!=1) {
      cout << "test failed\n";
      exit(1);
    }

    g++;

  thread_unlock(2);
}


int main(int argc, char *argv[]) {
  if (thread_libinit(launch, (void *) 3)) {
    cout << "thread_libinit failed\n";
    exit(1);
  }
}