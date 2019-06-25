/*
 * (list.c | 28 Nov 18 | Ahmad Maher)
 *
*/

#include <stdarg.h>
#include <stdlib.h>

#include "list.h"

List *list_append(List *list, List *tail) {
    List **p = &list;

    while (*p)
        p = &(*p)->tail;
    *p = tail;

    return list;
}

void *list_remove(List *list, int pos) {
    for (int i = 0; i < pos; i++)
        list = list->tail;
    
    List *rem = list->tail;
    void *x = rem->head;

    list->tail = rem->tail;
    free(rem);
    return x;
}

List *list_add(List *list, void *x) {
    List **p = &list;
    
    while (*p)
        p = &(*p)->tail;

    *p = malloc(sizeof (**p));
    (*p)->head = x;
    (*p)->tail = NULL;

    return list;
}

List *list_push(List *list, void *x) {
    List *cell = malloc(sizeof (*cell));
    cell->head = x;
    cell->tail = list;
    return cell;
}

List *list_pop(List *list, void **x) {
    if (list) {
        List *head = list->tail;
        if (x) *x = list->head;
        free(list);
        return head;
    }
    return list;
}

List *list_rev(List *list) {
    List *head = NULL;
    List *tail;

    for ( ; list; list = tail) {
        tail = list->tail;
        list->tail = head;
        head = list;
    }

    return head;
}

List *list_map(List *list, Map_Fn f) {
    List *head, **p = &head;

    for ( ; list; list = list->tail) {
        *p = malloc(sizeof (**p));
        (*p)->head = f(list->head);
        p = &(*p)->tail;
    }
    *p = NULL;
    return head;    
}

List *list_copy(List *list, Copy_Fn f) {
    return list_map(list, f);
}

List *list_filter(List *list, Pred_Fn f, Copy_Fn cpy) {
    List *head, **p = &head;

    for ( ; list; list = list->tail) {
        if (!f(list->head))
            continue;
        
        *p = malloc(sizeof (**p));
        (*p)->head = cpy(list->head);
        p = &(*p)->tail;
    }

    return head;
}

void *list_foldl(List *list, Fold_Fn f, void *b) {
    for ( ; list; list = list->tail)
        b = f(b, list->head);

    return b;
}

void *list_foldr(List *list, Fold_Fn f, void *b) {
    if (!list)
        return b;
    
    b = list_foldr(list->tail, f, b);
    b = f(list->head, b);
    return b;
}

void list_iter(List *list, Iter_Fn f) {
    for ( ; list; list = list->tail)
        f(list->head);
}

int list_exists(List *list, Pred_Fn f) {
    for ( ; list; list = list->tail)
        if (f(list->head)) return 1;

    return 0;
}

int list_forall(List *list, Pred_Fn f) {
    for ( ; list; list = list->tail)
        if (!f(list->head)) return 0;

    return 1;
}

int list_len(List *list) {
    int n;
    for (n = 0; list; list = list->tail)
        n++;

    return n;
}

List *list_of(void *x, ...) {
    List *list, **p = &list;
        
    va_list ap;
    va_start(ap, x);
    
    for ( ; x; x = va_arg(ap, void*)) {
        *p = malloc(sizeof (**p));
        (*p)->head = x;
        p = &(*p)->tail;
    }
    *p = NULL;

    va_end(ap);
    return list;
}

void **list_to_vec(List *list) {
    int len = list_len(list);
    void **vec = malloc((len+1) * sizeof (*vec));

    int i;
    for (i = 0; i < len; i++) {
        vec[i] = list->head;
        list = list->tail;
    }
    vec[i] = NULL;

    return vec;
}

void free_list(List **lp, Free_Fn f) {
    List *list;

    for ( ; *lp; *lp = list) {
        list = (*lp)->tail;
        if (f)
            f((*lp)->head);
        free(*lp);
    }
}
