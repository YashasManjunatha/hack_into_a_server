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

/* ----------------------------- CASE 4 TEST ----------------------------- */
// Creates numLoops number of threads that run the loop function with
// parent function calling loop as well
void case4(void *a) {
  int arg;
  arg = (long int) a;

  cout << "case4 called with arg " << arg << endl;
  int i = 0;
  while( i < 7 ){
    if (thread_create(loop, (void *) "child thread")) {
      cout << "thread_create failed\n";
      exit(1);
    }
    i++;
  }

  loop( (void *) "parent thread");
}

/* ----------------------------- MAIN FUNCTION ----------------------------- */
int main(int argc, char *argv[]) {
  if (thread_libinit(case4, (void *) 4)) {
    cout << "thread_libinit failed\n";
    exit(1);
  }
  exit(0);
}