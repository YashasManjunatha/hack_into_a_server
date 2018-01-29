#include <stdio.h>  // needed for size_t
#include <unistd.h> // needed for sbrk
#include <assert.h> // needed for asserts
#include "dmm.h"

/* You can improve the below metadata structure using the concepts from Bryant
 * and OHallaron book (chapter 9).
 */

typedef struct metadata {
  /* size_t is the return type of the sizeof operator. Since the size of an
   * object depends on the architecture and its implementation, size_t is used
   * to represent the maximum size of any object in the particular
   * implementation. size contains the size of the data object or the number of
   * free bytes
   */
  size_t size;
  struct metadata* true_next;
  struct metadata* true_prev;
  struct metadata* freelist_next;
  struct metadata* freelist_prev; 
} metadata_t;

/* freelist maintains all the blocks which are not in use; freelist is kept
 * sorted to improve coalescing efficiency 
 */

static metadata_t* freelist = NULL;

bool place_in_freelist(metadata_t* head) {
  metadata_t *freelist_head = freelist;
  while(freelist_head->freelist_next != NULL) {
    if(freelist_head->size >= head->size) {
      if(freelist_head->freelist_prev != NULL)
        freelist_head->freelist_prev->freelist_next = head;
      head->freelist_prev = freelist_head->freelist_prev;
      freelist_head->freelist_prev = head;
      head->freelist_next = freelist_head;
      return true;
    }
    freelist_head = freelist_head->freelist_next;
  }
  freelist_head->freelist_next = head;
  head->freelist_prev = freelist_head;
  return true;
}

metadata_t* find_fit(size_t requested_size) {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    if(freelist_head->size >= requested_size) {
      if(freelist_head->freelist_prev != NULL)
        freelist_head->freelist_prev->freelist_next = freelist_head->freelist_next;
      if(freelist_head->freelist_next != NULL)
        freelist_head->freelist_next->freelist_prev = freelist_head->freelist_prev;
      freelist_head->freelist_prev = NULL;
      freelist_head->freelist_next = NULL;
      return freelist_head;
    }
    freelist_head = freelist_head->freelist_next;
  }
  return NULL;
}

void* dmalloc(size_t numbytes) {
  /* initialize through sbrk call first time */
  if(freelist == NULL) { 			
    if(!dmalloc_init())
      return NULL;
  }

  assert(numbytes > 0);

  metadata_t *fit_head = find_fit(ALIGN(numbytes) + METADATA_T_ALIGNED);
  if (fit_head != NULL) {
    metadata_t *split_head = (metadata_t*) (((void*) (fit_head+1)) + ALIGN(numbytes));
    split_head->size = fit_head->size - ALIGN(numbytes) - METADATA_T_ALIGNED;
    split_head->true_prev = fit_head;
    split_head->true_next = fit_head->true_next;

    if(freelist == fit_head)
      freelist = split_head;
    else
      place_in_freelist(split_head);

    fit_head->size = ALIGN(numbytes);
    fit_head->true_next = split_head;
    fit_head->freelist_next = NULL;
    fit_head->freelist_prev = NULL;
    return (void*) (fit_head+1);
  }
	
  return NULL;
}

metadata_t* coalesc(metadata_t* head) {
  metadata_t *head_next = head->true_next;
  while(head_next != NULL && (head_next->freelist_next != NULL && head_next->freelist_prev != NULL)) {
    head->size = head->size + METADATA_T_ALIGNED + head_next->size;
    head->true_next = head_next->true_next;
    head_next->true_next->true_prev = head;
    head->freelist_next = head_next->freelist_next;
    head_next->freelist_prev = head;
    head_next = head->true_next;
  }
  metadata_t *head_prev = head->true_prev;
  while(head_prev != NULL && (head_prev->freelist_next != NULL && head_prev->freelist_prev != NULL)) {
    head_prev->size = head_prev->size + METADATA_T_ALIGNED + head->size;
    head_prev->true_next = head->true_next;
    if(head->true_next != NULL)
      head->true_next->true_prev = head_prev;
    head_prev->freelist_next = head->freelist_next;
    if(head->freelist_next != NULL)
      head->freelist_next->freelist_prev = head_prev;
    head = head_prev;
    head_prev = head_prev->true_prev;
  }
  return head;
}

void dfree(void* ptr) {
  place_in_freelist(coalesc(((metadata_t*) ptr)-1));
}

bool dmalloc_init() {

  /* Two choices: 
   * 1. Append prologue and epilogue blocks to the start and the
   * end of the freelist 
   *
   * 2. Initialize freelist pointers to NULL
   *
   * Note: We provide the code for 2. Using 1 will help you to tackle the 
   * corner cases succinctly.
   */

  size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
  /* returns heap_region, which is initialized to freelist */
  freelist = (metadata_t*) sbrk(max_bytes); 
  /* Q: Why casting is used? i.e., why (void*)-1? */
  if (freelist == (void *)-1)
    return false;
  freelist->true_next = NULL;
  freelist->true_prev = NULL;
  freelist->freelist_next = NULL;
  freelist->freelist_prev = NULL;
  freelist->size = max_bytes-METADATA_T_ALIGNED;
  return true;
}

/* for debugging; can be turned off through -NDEBUG flag*/
void print_freelist() {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p\t",
	  freelist_head->size,
	  freelist_head,
	  freelist_head->freelist_prev,
	  freelist_head->freelist_next);
    freelist_head = freelist_head->freelist_next;
  }
  DEBUG("\n");
}
