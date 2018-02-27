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

/* ----------------------------- MAIN FUNCTION ----------------------------- */
int main(int argc, char *argv[]) {
  if (thread_libinit(case2, (void *) 2)) {
    cout << "thread_libinit failed\n";
    exit(1);
  }
  exit(0);
}