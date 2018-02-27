#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>

using namespace std;

int g=0;

void loop(void *a) {
  thread_lock(1);
	 
  g++;

  thread_wait(1, 1);

  g++;
  
  thread_unlock(1);
}

void launch(void *a) {
  thread_lock(2);
  
    for (int i = 0; i < 5; i++) {

  	if (thread_create(loop, (void *) "child thread")) {
  		cout << "thread_create failed\n";
   		exit(1);
  	}
    thread_yield();

  }

  if (g != 5) {
    cout << "test failed\n";
    exit(1);
  } else {
    cout << "SUCCESS\n";
  }

  thread_wait(2,2);

  cout << "test failed";
  exit(1);


  thread_unlock(2);
}


int main(int argc, char *argv[]) {
  if (thread_libinit(launch, (void *) 3)) {
    cout << "thread_libinit failed\n";
    exit(1);
  }
}