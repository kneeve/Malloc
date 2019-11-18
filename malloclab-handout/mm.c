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
#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))
#define MAX_PAGE_SIZE 24000

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)      (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

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

typedef struct exp_node{
  struct exp_node *next;
  struct exp_node *prev;
} exp_node;

struct exp_node *exp_head;
typedef size_t block_header;
typedef size_t block_footer;
void *current_avail = NULL;
int current_avail_size = 0;
size_t previous_size;

static void *grow_heap(size_t size);
static void set_allocated(void *bp, size_t size);
static void *coalesce(void *bp);

static void add(void* bp);
static void delete(void *bp);
static void *find_fit(size_t size);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  exp_head = NULL;
  grow_heap(mem_pagesize());
  
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  if (size <= 0)
    return NULL;
  
  void* bp = NULL;
  size_t full_size = ALIGN(size + OVERHEAD);

  if((bp = find_fit(full_size)))
    set_allocated(bp, full_size);
  else if ((bp = grow_heap(full_size))) {
    bp = exp_head;
    set_allocated(bp, full_size);
  }

  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  // PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));
  // PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));

  void *bp = coalesce(ptr);

  if ((GET_SIZE((HDRP(PREV_BLKP(bp)))) == OVERHEAD) && 
  (GET_SIZE(FTRP(bp) + DSIZE) == 0)) {
    if (GET_SIZE(HDRP(bp)) >= (mem_pagesize() * 12)) {
      delete(bp);
      mem_unmap(bp - 32, GET_SIZE(HDRP(bp)) + 32);
    }
  }
}

/*
* -------------------- HELPERS --------------------
*/

/*
* When there is not enough available space, grow the heap.
*/
static void *grow_heap(size_t size) {
  void *bp;
  size_t req_size       = PAGE_ALIGN(size + 32);
  size_t calc_prev_size = PAGE_ALIGN((previous_size * 2) + 32);
  size_t use_size = MAX(req_size, calc_prev_size);

  // If the requested size is within the max value, update previous size
  if (use_size <= MAX_PAGE_SIZE)
    previous_size = use_size;
  else
    use_size = previous_size;
  
  if (!(bp = mem_map(PAGE_ALIGN(use_size + 32))))
    return NULL;

  PUT(bp, 0);
  PUT(bp + 8, PACK(OVERHEAD, 1));
  PUT(bp + 16, PACK(OVERHEAD, 1));

  // Header
  PUT(bp + 24, PACK((use_size - 32), 0));
  // Footer
  PUT(FTRP(bp + 32), PACK((use_size - 32), 0));

  // Terminating block
  PUT((FTRP(bp + 32) + WSIZE), PACK(0, 1));
  
  add(bp + 32);
  return (bp + 32);
}

/*
* Change allocated bit to allocated and does splitting
*/
static void set_allocated(void *bp, size_t size) {
  if (!bp)
    return;
    
  size_t extra_size = GET_SIZE(HDRP(bp)) - size;

  if (extra_size > 32) {
    delete(bp);
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(extra_size, 0));
    add(NEXT_BLKP(bp));
  }
  else {
    
    PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    delete(bp);
  }
}

/*
* Uses boundary tag coalescing to merge free blocks.
*/
static void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  // Prev and next allocated
  if (prev_alloc && next_alloc) {
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
    add(bp);
  }

  // Prev allocated, next free
  else if (prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    delete(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    add(bp);
  }

  // Prev free, next allocated
  else if (!prev_alloc && next_alloc) {
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  // Prev and next free
  else {
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
    delete(NEXT_BLKP(bp));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  return bp;
}

static void add(void *bp) {
  if (bp == NULL)
    return;
  
  exp_node *node = (exp_node*)bp;
  node->next = exp_head;
  
  if(exp_head)
    exp_head->prev = node;

  node->prev = NULL;
  exp_head = node;
}

static void delete(void *bp) {
  if (bp == NULL)
    return;

  exp_node *node = (exp_node*)bp;

  if (node->prev == NULL) {
    if (node->next == NULL) {
      exp_head = NULL;
    }
    else {
      exp_head = node->next;
      exp_head->prev = NULL;
    }
  }
  else {
    if (node->next == NULL) {
      node->prev->next = NULL;
    }
    else {
      node->prev->next = node->next;
      node->next->prev = node->prev;
    }
  }
}

/*
* Implements a first fit search
*/
static void *find_fit(size_t size) {
  struct exp_node *node = exp_head;

  while (node) {
    if (GET_SIZE(HDRP(node)) < size)
      node = node->next;
    else
      return (void*)node;
  }

  return NULL; // No fit
}