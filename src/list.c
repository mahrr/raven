/*
 * (list.c | 28 Nov 18 | Ahmad Maher)
 *
*/

#include <stdlib.h>
#include <assert.h>

#include "list.h"
#include "alloc.h"

list *append_list(list *l, void *obj) {
    if (l == NULL) {
        l = make(l, R_FIRS);
        l->obj = obj;
        l->link = NULL;
        return l;
    }

    list *lp = l;
    while (lp->link != NULL) lp = lp->link;

    list *new = make(new, R_FIRS);
    lp->link = new;
    new->link = NULL;
    new->obj = obj;
    return l;
}

 list *remove_obj(list *l, void *obj) {
    /* list can't be empty */
    assert(l != NULL);

    /* first object in the list */
    if (l->obj == obj) {
        l = l->link;
        return l;
    }

    list *lp = l;
    while (lp->link != NULL && lp->link->obj != obj)
        lp = lp->link;

    /* object not found */
    if (lp->link == NULL)
        return NULL;

    lp->link = lp->link->link;
    return l;
}

/* the current iterated list */
static list *curr_list = NULL;
/* the current object in the list */
static list *curr_block = NULL;

void *iter_list(list *l) {
    assert(l != NULL);

    if (curr_list == NULL || curr_list != l) {
        curr_list = l;
        curr_block = l;
    }

    if (curr_block == NULL)
        return NULL;

    void *ret_obj = curr_block->obj;
    curr_block = curr_block->link;
    return ret_obj;
}

long len_list(list *l) {
    if (l == NULL) return 0;

    long n = 0;
    while (l->link) {
        n++;
        l = l->link;
    }

    return n;
}

void *list_to_vec(list *l, unsigned reg) {
    assert(l != NULL);
    
    long size;

    size = len_list(l) * sizeof (void*);
    void **vec = alloc(size, reg);

    list *lp = l;
    void *obj;
    int i = 0;

    while ((obj = iter_list(lp)))       
        vec[i++] = obj;

    vec[++i] = NULL;
    return vec;
}
