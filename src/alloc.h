/*
 * (alloc.h | 15 Nov 18 | Ahmad Maher)
 *
 * A simple objects lifetime based allocator.
 *
 * In this scheme, the cost of deallocation is negligible just
 * basic pointer arthimitic. Allocation is trivial operation and
 * incurs no deallocation obligation, so deallocation can't be forgotten.
 *
 * The scheme works with three regions(the number could be changed)
 * every region is a linked list of memory blocks. 
 *
 * Allocations occures in the region blocks. The allocator searches
 * for a block with enough space for the object if the current pointed at 
 * block have enough space, it allocates in this block. otherwise,
 * it first searches in the freeblocks which is a list of deallocated 
 * blocks, if there is a block with enough space, it uses it. 
 * if not, the allocater allocates a new block in the demanded region.
 * 
 * Deallocations of an region just addes its blocks to the freeblocks
 * list and reinitializing it to point to a NULL block.
 *
*/ 

#ifndef alloc_h
#define alloc_h

#include <string.h>

#define REG_NUM 3     /* regions number */
#define R_PERM 0      /* permanent Region */
#define R_FIRS 1      /* first region */
#define R_SECN 2      /* secondary region */

#define make(p, reg) (alloc(sizeof *(p), (reg)))
#define make_init(p, reg) memset(make((p), (reg)), 0, sizeof *(p))

#define roundup(x, n) (((x) + ((n)-1)) & (~((n)-1)))

/* allocate/deallocate region by its identifier reg */
extern void  *alloc(unsigned long n, unsigned reg);
extern void dealloc(unsigned reg);

/* Region is a linked list with large blocks of memory.
   Each block begins with a header defined by (struct block) */
typedef struct block {
    struct block *next; /* the next block in the list */
    char *avail;        /* the first free location within the block */
    char *limit;        /* the end of the block */
} block;

#endif
