#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>

using namespace std;

/* ----------------------------- CASE 9 TEST ----------------------------- */
// wait without holding a lock. 
void case9(void *a) {
  return wait(1, 1);
}

/* ----------------------------- MAIN FUNCTION ----------------------------- */
int main(int argc, char *argv[]) {
  if (thread_libinit(case9, (void *) 5) != -1) {
    exit(1);
  }
    exit(0);
}