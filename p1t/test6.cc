#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>

using namespace std;

int g=0;

void test(void *a) {
  char *id;
  int i;

  id = (char *) a;
  cout << "test called with id " << (char *) id << endl;

  for (i=0; i<5; i++) {
    cout << id << ":\t" << i << "\n";
    if (thread_lock(g)) {
      cout << "thread_lock tried to acquire the same lock\n";
      exit(1);
    }
  }
}

/* ----------------------------- TEST 6 TEST ----------------------------- */
// Tests to see if trying to acquire lock you already have will be destroyed
void case1(void *a) {
  int arg;
  arg = (long int) a;

  cout << "case1 called with arg " << arg << endl;
  if (thread_create(test, (void *) "child thread")) {
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