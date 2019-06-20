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

struct List {
    Block entry;  /* the first block of the list */
    Block end;    /* the end block of the list */
    Block curr;   /* the current block for the iteration */
    long count;   /* the number of blocks in the list */
    Region_N reg; /* the region which the list been allocated in */
};

List List_new(Region_N reg) {
    List l = make(l, reg);
    l->entry = NULL;
    l->end = NULL;
    l->curr = NULL;
    l->count = 0;
    l->reg = reg;

    return l;
}

List List_append(List l, void *obj) {
    assert(l != NULL);
    
    /* empty list */
    if (l->entry == NULL) {
        l->entry = make(l->entry, l->reg);
        l->entry->object = obj;
        l->entry->link = NULL;
        l->end = l->entry;
        l->curr = l->entry;
        l->count++;
        return l;
    }

    Block b = l->end;
    
    b->link = make(b->link, l->reg);
    b->link->object = obj;
    b->link->link = NULL;
    l->end = b->link;
    l->count++;

    return l;
}

void *List_curr(List l) {
    assert(l != NULL);

    return l->curr ? l->curr->object : NULL;
}

void *List_peek(List l) {
    assert(l != NULL);

    Block c = l->curr;
    Block n;
    
    if (c && (n = c->link)) return n->object;
    return NULL;
}

void *List_iter(List l) {
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

void List_unwind(List l) {
    assert(l != NULL);
    l->curr = l->entry;
}

long List_len(List l) {
    assert(l != NULL);
    return l->count;
}

void *Listo_vec(List l, Region_N reg) {
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
