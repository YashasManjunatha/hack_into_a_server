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

/* ----------------------------- MAIN FUNCTION ----------------------------- */
int main(int argc, char *argv[]) {
  if (thread_libinit(case1, (void *) 1)) {
    cout << "thread_libinit failed\n";
    exit(1);
  }
  exit(0);
}