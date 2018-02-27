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
      cout << "If this has happened then there is an error.\n";
      exit(1);
    }
    if( thread_unlock(g) ){
      cout << "If this has happened then there is an error.\n";
      exit(1);
    }
  }
  if( thread_unlock(g) ){
    cout << "Test8 should have successfully displayed that the thread tried to " \
            "unlock a lock it didn't have!\n";
    exit(1);
  }
  cout << "The function \"test\" is exiting!\n";
}

/* ----------------------------- TEST 6 TEST ----------------------------- */
// Tests to throw error if thread tries to unlock lock it doesn't own.
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