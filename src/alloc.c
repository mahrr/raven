/*
 * (alloc.c | 15 Nov 18 | Ahmad Maher)
 *
 * See alloc.h for full description.
*/

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>


#include "alloc.h"
#include "error.h"

/* strictest alignment type */
typedef max_align_t align;

/* ensures that avail in a property align address */
typedef union {
    block b;
    align a;
} header;

/* debugging implementation with malloc/free */
#ifdef DEBUG

static struct {
    void *memp;
    size_t size;
} *region[REG_NUM] = {{NULL, 0}, {NULL, 0}, {NULL, 0}};

void *alloc(unsinged long n, unsigned reg) {
    void *p = region[reg]->memp;
    size_t curr_size = region[reg]->size;

    p = realloc(p, curr_size + n);
    region[reg]->size += n;
}

void dealloc(unsigned reg) {
    free(region[reg]->memp);
    region[reg]->size = 0;
}
    
/* actual implementation */
#else

/* ## alloc - Data */

/* this implementation works with 3 regions */
static block first[] = { {NULL}, {NULL}, {NULL} },
            *region[] = {&first[0], &first[1], &first[2]};

/* list of the freed blocks which allocate 
   reuses instead of allocating new blocks */
static block *freeblocks;

/* ## alloc - Functions */

/* the space allocated with every blocks for future allocation
   besides the n bytes needed */
#define PLUS_SPACE 16384

void *alloc(unsigned long n, unsigned reg) {
    assert(n > 0);
    assert(reg < REG_NUM);
    
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
                fatal_error(1, "not enough memory\n");
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

#endif
