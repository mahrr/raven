/*
 * (alloc.c | 15 Nov 18 | Ahmad Maher)
 *
 * See alloc.h for full description.
 *
*/

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>


#include "alloc.h"
#include "error.h"

/* strictest alignment type */
#if __STDC_VERSION__ <= 199901L

typedef union {
    long double ld;
    double d;
    float f;
    long l;
    int i;
    void *p;
} align;

#elif __STDC_VERSION__ == 201112L

typedef max_align_t align;

#endif

/* Region is a linked list with large blocks of memory.
   Each block begins with a header defined by (struct block) */
typedef struct block {
    struct block *next; /* the next block in the list */
    char *avail;        /* the first free location within the block */
    char *limit;        /* the end of the block */
} block;

/* ensures that avail in a property align address */
typedef union {
    block b;
    align a;
} header;

/* this implementation works with 3 regions */
static block first[] = { {NULL}, {NULL}, {NULL} },
            *region[] = {&first[0], &first[1], &first[2]};

/* list of the freed blocks which allocate 
   reuses instead of allocating new blocks */
static block *freeblocks;

#define roundup(x, n) (((x) + ((n)-1)) & (~((n)-1)))

/* the space allocated with every blocks for future allocation
   besides the n bytes needed */
#define PLUS_SPACE 16384

void *alloc(unsigned long n, unsigned reg) {
    assert(n > 0);
    assert(reg < 3);
    
    struct block *ap;

    ap = region[reg];
    n = roundup(n, sizeof(align));

    while (ap->avail + n > ap->limit) {
        if ((ap->next = freeblocks) != NULL) {
            /* get new block from the freed blocks if
               available before allocating any new blocks */
            freeblocks = freeblocks->next;
            ap = ap->next;
        } else {
            /* allocate new block */
            unsigned m = sizeof(header) + n + PLUS_SPACE;
            ap->next = malloc(m);
            ap = ap->next;
            if (ap == NULL) {
                /* @@ for now */
                fatal_err(1, "not enough memory\n");
            }
            ap->limit = (char *)ap + m;
            ap->avail = (char *)((header *)ap + 1);
            ap->next = NULL;
            region[reg] = ap;
        } 
    }
    ap->avail += n;
    return ap->avail-n;
}

void dealloc(unsigned reg) {
    region[reg]->next = freeblocks;
    freeblocks = first[reg].next;
    first[reg].next = NULL;
    region[reg] = &first[reg];
}
