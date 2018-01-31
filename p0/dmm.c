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
  struct metadata* true_prev;
  struct metadata* true_next;
  struct metadata* free_prev;
  struct metadata* free_next;
} metadata_t;

/* freelist maintains all the blocks which are not in use; freelist is kept
 * sorted to improve coalescing efficiency 
 */

static metadata_t* freelist = NULL;
static metadata_t* truelist = NULL;

void place_in_freelist(metadata_t* head) {
  if (freelist == NULL){
    freelist = head;
    freelist->free_next = NULL;
    freelist->free_prev = NULL;
    return;
  }
  metadata_t *freelist_head = freelist;
  if(freelist_head->size >= head->size) {
    head->free_next = freelist_head;
    freelist_head->free_prev = head;
    freelist = head;
  }
  else {
    while(freelist_head->free_next != NULL && freelist_head->free_next->size < head->size) {
      freelist_head = freelist_head->free_next;
    }
    head->free_next = freelist_head->free_next;
    if(head->free_next != NULL)
      head->free_next->free_prev = head;
    freelist_head->free_next = head;
    head->free_prev = freelist_head;
  }
}

// bool place_in_freelist(metadata_t* head) {
//   metadata_t *freelist_head = freelist;
//   if (freelist_head->free_next == NULL) {
//     if (freelist_head->size >= head->size) {
//       head->free_next = freelist_head;
//       head->free_prev = NULL;//freelist_head->free_prev;
//       freelist_head->free_prev = head;
//       freelist = head;
//     } else {
//       head->free_prev = freelist_head;
//       head->free_next = NULL;//freelist_head->free_next;
//       freelist_head->free_next = head;
//     }
//     return true;
//   } else {
//     while(freelist_head->free_next != NULL) {
//       if(freelist_head->size >= head->size) {
//         if(freelist_head->free_prev != NULL)
//           freelist_head->free_prev->free_next = head;
//         head->free_prev = freelist_head->free_prev;
//         freelist_head->free_prev = head;
//         head->free_next = freelist_head;
//         return true;
//       }
//       freelist_head = freelist_head->free_next;
//     }
//     /*if (freelist_head->free_next == NULL) {
//       if(freelist_head->size >= head->size) {
//         if(freelist_head->free_prev != NULL)
//           freelist_head->free_prev->free_next = head;
//         head->free_prev = freelist_head->free_prev;
//         freelist_head->free_prev = head;
//         head->free_next = freelist_head;
//         return true;
//       } else {
//         freelist_head->free_next = head;
//         head->free_prev = freelist_head;
//         head->free_next = NULL;
//         return true;
//       }
//     }*/
//     freelist_head->free_next = head;
//     head->free_prev = freelist_head;
//     head->free_next = NULL;
//     return true;
//   }
//   return false;
// }

metadata_t* find_fit(size_t requested_size) {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    if(freelist_head->size >= requested_size) {
      /*
      if(freelist_head->free_prev != NULL)
        freelist_head->free_prev->free_next = freelist_head->free_next;
      if(freelist_head->free_next != NULL)
        freelist_head->free_next->free_prev = freelist_head->free_prev;
      */
      //freelist_head->free_prev = NULL;
      //freelist_head->free_next = NULL;
      return freelist_head;
    }
    freelist_head = freelist_head->free_next;
  }
  return NULL;
}

void remove_from_freelist(metadata_t* head) {
  if(freelist == head)
    freelist = freelist->free_next;
  if(head->free_prev != NULL) {
    head->free_prev->free_next = head->free_next;
    //head->free_prev = NULL;
  }
  if(head->free_next != NULL) {
    head->free_next->free_prev = head->free_prev;
    //head->free_next = NULL;
  }
  head->free_prev = NULL;
  head->free_next = NULL;
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

    if (freelist == fit_head && freelist->free_next != NULL) {
      freelist = freelist->free_next;
      remove_from_freelist(fit_head);
    }
    if (freelist == fit_head && freelist->free_next == NULL) {
      freelist = split_head;
      remove_from_freelist(fit_head);
      split_head->free_prev = NULL;//fit_head->free_prev;
      split_head->free_next = NULL;//fit_head->free_next;
    } else {
      remove_from_freelist(fit_head);
      place_in_freelist(split_head);
    }

    /*
    if(freelist->free_next == NULL) {
      freelist = split_head;
      split_head->free_prev = NULL;//fit_head->free_prev;
      split_head->free_next = NULL;//fit_head->free_next;
    }
    else
      place_in_freelist(split_head);
      */

    fit_head->size = ALIGN(numbytes);
    fit_head->true_next = split_head;
    fit_head->free_next = NULL;
    fit_head->free_prev = NULL;
    return (void*) (fit_head+1);
  }
  return NULL;
}

/*metadata_t* coalesce(metadata_t* head) {
  metadata_t *head_next = head->true_next;
  while(head_next != NULL && (head_next->free_next != NULL || head_next->free_prev != NULL)) {
    if (freelist == head_next) {
      if (freelist->free_next != NULL)
        freelist = freelist->free_next;
      //else
        //freelist = head;
    }
    head->size = head->size + METADATA_T_ALIGNED + head_next->size;
    if (freelist == head && head->free_next != NULL){
      if (head->free_next->size <= head->size)
        freelist = freelist->free_next;
    }
    head->true_next = head_next->true_next;
    if(head_next->true_next != NULL)
      head_next->true_next->true_prev = head;
    //head->free_next = head_next->free_next;
    if(head_next->free_next != NULL)
      head_next->free_next->free_prev = head_next->free_prev;
    if(head_next->free_prev != NULL)
      head_next->free_prev->free_next = head_next->free_next;
    head_next = head->true_next;
  }

  metadata_t *head_prev = head->true_prev;
  while(head_prev != NULL && (head_prev->free_next != NULL || head_prev->free_prev != NULL)) {
    if (freelist == head) {
      if (freelist->free_next != NULL)
        freelist = head->free_next;
      //else
        //freelist = head_prev;
    }
    head_prev->size = head_prev->size + METADATA_T_ALIGNED + head->size;
    if (freelist == head_prev && head_prev->free_next != NULL) {
      if (head_prev->free_next->size <= head_prev->size)
        freelist = freelist->free_next;
    }
    head_prev->true_next = head->true_next;
    if(head->true_next != NULL)
      head->true_next->true_prev = head_prev;
    //head_prev->free_next = head->free_next;
    if(head->free_next != NULL)
      head->free_next->free_prev = head->free_prev;
    if(head->free_prev != NULL)
      head->free_prev->free_next = head->free_next;
    head = head_prev;
    head_prev = head_prev->true_prev;
  }
  return head;
}*/

metadata_t* coalesce(metadata_t* head) {
  metadata_t *head_next = head->true_next;
  while(head_next != NULL && (head_next->free_next != NULL || head_next->free_prev != NULL)) {
    remove_from_freelist(head_next);
    remove_from_freelist(head);

    head->size = head->size + METADATA_T_ALIGNED + head_next->size;
    head->true_next = head_next->true_next;
    if(head->true_next != NULL)
      head->true_next->true_prev = head;

    head_next->true_prev = NULL;
    head_next->true_next = NULL;

    place_in_freelist(head);
    head_next = head_next->true_next;
  }

  metadata_t *head_prev = head->true_prev;
  while(head_prev != NULL && (head_prev->free_next != NULL || head_prev->free_prev != NULL)) {
    remove_from_freelist(head);
    remove_from_freelist(head_prev);

    head_prev->size = head_prev->size + METADATA_T_ALIGNED + head->size;
    head_prev->true_next = head->true_next;
    if(head_prev->true_next != NULL)
      head_prev->true_next->true_prev = head_prev;

    head->true_prev = NULL;
    head->true_next = NULL;

    place_in_freelist(head_prev);
    head = head_prev;
    head_prev = head_prev->true_prev;
  }
  return head;
}

void dfree(void* ptr) {
  metadata_t *head = (metadata_t*) ptr - 1;
  place_in_freelist(head);
  coalesce(head);
  //place_in_freelist(((metadata_t*) ptr)-1);
  //place_in_freelist(coalesce(((metadata_t*) ptr)-1));
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
  truelist = freelist;
  /* Q: Why casting is used? i.e., why (void*)-1? */
  if (freelist == (void *)-1)
    return false;
  freelist->true_next = NULL;
  freelist->true_prev = NULL;
  freelist->free_next = NULL;
  freelist->free_prev = NULL;
  freelist->size = max_bytes-METADATA_T_ALIGNED;
  return true;
}

/* for debugging; can be turned off through -NDEBUG flag*/
void print_freelist() {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    DEBUG("\tFreelist Size:%zd, Head:%p, TruePrev:%p, TrueNext:%p, FreePrev:%p, FreeNext:%p\t",
	  freelist_head->size,
	  freelist_head,
    freelist_head->true_prev,
    freelist_head->true_next,
	  freelist_head->free_prev,
	  freelist_head->free_next);
    freelist_head = freelist_head->free_next;
  }
  DEBUG("\n");
}

/* for debugging; can be turned off through -NDEBUG flag*/
void print_truelist() {
  metadata_t *truelist_head = truelist;
  while(truelist_head != NULL) {
    DEBUG("\tTruelist Size:%zd, Head:%p, TruePrev:%p, TrueNext:%p, FreePrev:%p, FreeNext:%p\t",
    truelist_head->size,
    truelist_head,
    truelist_head->true_prev,
    truelist_head->true_next,
    truelist_head->free_prev,
    truelist_head->free_next);
    truelist_head = truelist_head->true_next;
  }
  DEBUG("\n");
}