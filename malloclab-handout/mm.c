/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * Kaelin Hoang, u0984462
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* Basic constants and macros */
/* Always use 16-byte alignment */
#define ALIGNMENT 16
#define DSIZE     16        // Double word size in bytes
#define CHUNKSIZE (1 << 12) // Extend heap by this amount in bytes
#define WSIZE     8         // Word and header/footer size in bytes
#define OVERHEAD  (sizeof(block_header) + sizeof(block_footer))

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)      (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  ((block_header *) (p))->size
#define GET_ALLOC(p) ((block_header *) (p))->allocated

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - OVERHEAD)))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

typedef struct {
  struct free_node* next;
  struct free_node* prev;
} free_node;

typedef struct {
  size_t size;
  char allocated;
} block_header;

typedef struct {
  size_t size;
  int filler;
} block_footer;

void *current_avail = NULL;
int current_avail_size = 0;
struct free_node *head;

void *first_bp;
void *heapp;


//static void *coalesce(void *bp);
//static void *extend_heap(size_t words);
static void set_allocated(void *b, size_t size);
static void extend(size_t s);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  // current_avail = NULL;
  // current_avail_size = 0;

  //Initilize the heap
  // heapp = mem_map(PAGE_ALIGN(4 * WSIZE));
  // PUT(heapp, 0);
  // PUT(heapp + (1 * WSIZE), PACK(DSIZE, 1));
  // PUT(heapp + (2 * WSIZE), PACK(DSIZE, 1));
  // PUT(heapp + (3 * WSIZE), PACK(0, 1));
  // heapp += (2 * WSIZE);

  // extend(CHUNKSIZE/WSIZE);

  first_bp = mem_map(PAGE_ALIGN(sizeof(block_header)));
  PUT(HDRP(first_bp), PACK(0, 1));

  mm_malloc(0);
  
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  // int newsize = ALIGN(size);
  // void *p;
  
  // if (current_avail_size < newsize) {
  //   current_avail_size = PAGE_ALIGN(newsize);
  //   current_avail = mem_map(current_avail_size);
  //   if (current_avail == NULL)
  //     return NULL;
  // }

  // p = current_avail;
  // current_avail += newsize;
  // current_avail_size -= newsize;
  
  // return p;

  int new_size = ALIGN(size);
  void *bp = first_bp;

  while (GET_SIZE(HDRP(bp)) != 0) {
    if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= new_size)) {
      set_allocated(bp, new_size);
      return bp;
    }

    bp = NEXT_BLKP(bp);
  }

  extend(new_size);
  set_allocated(bp, new_size);
  return bp;
}

/*
 * mm_free - Coalesce blocks if possible
 */
void mm_free(void *ptr)
{
  // size_t size = GET_SIZE(HDRP(ptr));

  // PUT(HDRP(ptr), PACK(size, 0));
  // PUT(FTRP(ptr), PACK(size, 0));
  // coalesce(ptr);

  GET_ALLOC(HDRP(ptr)) = 0;
}

/*
* ---------------- HELPERS ---------------
*/

/*
* Uses boundary tag coalescing to merge free blocks.
*/
// static void *coalesce(void *bp) {
//   size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
//   size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
//   size_t size = GET_SIZE(HDRP(bp));

//   // Prev and next allocated
//   if (prev_alloc && next_alloc) {
//     return bp;
//   }

//   // Prev allocated, next free
//   else if (prev_alloc && !next_alloc) {
//     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
//     PUT(HDRP(bp), PACK(size, 0));
//     PUT(FTRP(bp), PACK(size, 0));
//   }

//   // Prev free, next allocated
//   else if (!prev_alloc && next_alloc) {
//     size += GET_SIZE(HDRP(PREV_BLKP(bp)));
//     PUT(FTRP(bp), PACK(size, 0));
//     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
//     bp = PREV_BLKP(bp);
//   }

//   // Prev and next free
//   else {
//     size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
//     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
//     PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
//     bp = PREV_BLKP(bp);
//   }

//   return bp;
// }

// static void *extend_heap(size_t words) {
//   size_t size = PAGE_ALIGN(words);
//   void *bp = mem_map(size);
//   PUT(bp, 0); // Alignment padding
  
//   return bp;
// }

void extend(size_t new_size) {
  size_t page_size = PAGE_ALIGN(new_size);
  void *bp = mem_map(page_size);

  PUT(HDRP(bp), PACK(page_size, 0));

  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
}

/*
* Change allocated bit to allocated and does splitting
*/
void set_allocated(void *bp, size_t size) {
  size_t extra_size = GET_SIZE(HDRP(bp)) - size;

  if (extra_size > ALIGN(1 + OVERHEAD)) {
    PUT(HDRP(bp), PACK(size, 1));
    //PUT(FTRP(bp), PACK(size, 1));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
    //PUT(FTRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
  }
}
