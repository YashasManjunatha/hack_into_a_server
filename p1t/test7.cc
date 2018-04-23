#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>

using namespace std;

/* ----------------------------- CASE 7 TEST ----------------------------- */
// make sure all thread_library calls fail when called before thread_libinit


void foo(void *bar) {
  int i = 1;
  int j = i + 1;
}

/* ----------------------------- MAIN FUNCTION ----------------------------- */
int main(int argc, char *argv[]) {
  if (thread_create(foo, (void *) 1) != -1) {
    exit(1);
  }
  if (thread_yield() != -1) {
    exit(1);
  }

  if (thread_lock(1) != -1) {
    exit(1);
  }

  if (thread_unlock(1) != -1) {
    exit(1);
  }

  if (thread_wait(1, 2) != -1) {
    exit(1);
  }
  
  if (thread_signal(1, 2) != -1) {
    exit(1);
  }

  if (thread_broadcast(1, 2) != -1) {
    exit(1); 
  }
  cout << "library successfully throws errors when called without libinit.\n";
  exit(0);
}