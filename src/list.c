/*
 * (list.c | 28 Nov 18 | Ahmad Maher)
 *
*/

#include <stdlib.h>
#include <assert.h>

#include "list.h"
#include "alloc.h"
#include "error.h"

typedef struct Block *Block;

struct Block {
    void *object;
    Block link;
};

struct List_T {
    Block entry;  /* the first block of the list */
    Block curr;   /* the current block for the iteration */
    long count;   /* the number of blocks in the list */
    Region_N reg; /* the region which the list been allocated in */
};

List_T List_new(Region_N reg) {
    List_T l = make(l, reg);
    l->entry = NULL;
    l->curr = NULL;
    l->count = 0;
    l->reg = reg;

    return l;
}

List_T List_append(List_T l, void *obj) {
    assert(l != NULL);
    
    /* empty list */
    if (l->entry == NULL) {
        l->entry = make(l->entry, l->reg);
        l->entry->object = obj;
        l->entry->link = NULL;
        l->curr = l->entry;
        l->count++;
        return l;
    }

    Block b = l->entry;
    /* get to the end of the list */
    while (b->link != NULL) b = b->link;
    
    b->link = make(b->link, l->reg);
    b->link->object = obj;
    b->link->link = NULL;
    l->count++;

    return l;
}

void *List_iter(List_T l) {
    assert(l != NULL);
    
    /* reaching the end of the list causes unwinding */
    if (l->curr == NULL) {
        l->curr = l->entry;
        return NULL;
    }

    void *object = l->curr->object;
    l->curr = l->curr->link;
    
    return object;
}

void List_unwind(List_T l) {
    assert(l != NULL);
    l->curr = l->entry;
}

long List_len(List_T l) {
    assert(l != NULL);
    return l->count;
}

void *List_to_vec(List_T l, Region_N reg) {
    assert(l != NULL);

    long size = List_len(l) * sizeof (void*);
    void **vec = alloc(size, reg);

    int i = 0;
    void *obj;
    while ((obj = List_iter(l)))
        vec[i++] = obj;

    vec[++i] = NULL;
    return vec;
}
