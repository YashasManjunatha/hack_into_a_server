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
  bool free;
  struct metadata* next;
  struct metadata* prev; 
} metadata_t;

/* freelist maintains all the blocks which are not in use; freelist is kept
 * sorted to improve coalescing efficiency 
 */

static metadata_t* freelist = NULL;

metadata_t* find_fit(size_t requested_size) {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    if(freelist_head->size >= requested_size && freelist_head->free) {
      return freelist_head;
    }
    freelist_head = freelist_head->next;
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
    split_head->prev = fit_head;
    split_head->next = fit_head->next;
    if (fit_head->next != NULL)
      fit_head->next->prev = split_head;
    split_head->free = true;

    fit_head->size = ALIGN(numbytes);
    fit_head->next = split_head;
    fit_head->free = false;
    return (void*) (fit_head+1);
  }
	
  return NULL;
}


metadata_t* coalesce(metadata_t* head) {
  metadata_t *head_next = head->next;
  while(head_next != NULL && head_next->free) {

    head->size = head->size + METADATA_T_ALIGNED + head_next->size;
    head->next = head_next->next;
    if(head->next != NULL)
      head->next->prev = head;

    head_next->prev = NULL;
    head_next->next = NULL;

    head_next = head_next->next;
  }

  metadata_t *head_prev = head->prev;
  while(head_prev != NULL && head_prev->free) {
    
    head_prev->size = head_prev->size + METADATA_T_ALIGNED + head->size;
    head_prev->next = head->next;
    if(head_prev->next != NULL)
      head_prev->next->prev = head_prev;

    head->prev = NULL;
    head->next = NULL;

    head = head_prev;
    head_prev = head_prev->prev;
  }
  return head;
}


void dfree(void* ptr) {
  metadata_t *head = (metadata_t*) ptr - 1;
  head->free = true;
  coalesce(head);
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
  freelist->free = true;
  freelist->next = NULL;
  freelist->prev = NULL;
  freelist->size = max_bytes-METADATA_T_ALIGNED;
  return true;
}

/* for debugging; can be turned off through -NDEBUG flag*/
void print_freelist() {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    DEBUG("\tFreelist Size:%zd, Head:%p, Free?%s, Prev:%p, Next:%p\t",
	  freelist_head->size,
	  freelist_head,
    freelist_head->free ? "true" : "false",
	  freelist_head->prev,
	  freelist_head->next);
    freelist_head = freelist_head->next;
  }
  DEBUG("\n");
}
