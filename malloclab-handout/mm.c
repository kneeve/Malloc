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
/* always use 16-byte alignment */
#define ALIGNMENT 16
#define DSIZE     16 // Double word size in bytes
#define CHUNKSIZE (1 << 12) // Extend heap by this amount in bytes
#define WSIZE     8 // Word and header/footer size in bytes

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
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

void *current_avail = NULL;
int current_avail_size = 0;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  current_avail = NULL;
  current_avail_size = 0;
  
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  int newsize = ALIGN(size);
  void *p;
  
  if (current_avail_size < newsize) {
    current_avail_size = PAGE_ALIGN(newsize);
    current_avail = mem_map(current_avail_size);
    if (current_avail == NULL)
      return NULL;
  }

  p = current_avail;
  current_avail += newsize;
  current_avail_size -= newsize;
  
  return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}
