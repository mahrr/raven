/*
 * (list.h | 28 Nov 18 | Ahmad Maher)
 *
 * simple linked list.
 *
*/

#include "alloc.h"

#ifndef list_h
#define list_h

typedef struct List *List;
#define T List

/* 
   NOTE:
   None of these function that expect a list as an argument
   can be passed NULL List. 
*/

/* return a new empty list allocated on reg region. */
extern T List_new(Region_N reg);

/* appends a list l with object obj. */
extern T List_append(T l, void *obj);

/* iterate over a list l and return the next object or NULL on finish.
   The next call after the iteration ends will unwind the list to the 
   beginning. 
   It doesn't support nested iteration for the same list 
   (i.e. the nested iteration will continue from the outer iteration).*/
extern void *List_iter(T l);

/* return the current object of the iteration process without any
   incrementation and NULL on finish. */
extern void *List_curr(T l);

/* return the next object of the iteration process without any
   incrementation and NULL on finish. */
extern void *List_peek(T l);

/* unwind the list iteration to the beginning of the list. */
extern void List_unwind(T l);

/* returns the lenght of a list l. */
extern long List_len(T l);

/* convert a list to an continuation vector allocated on
   region reg with terminated null. */
extern void *Listo_vec(T l, Region_N reg);

#undef T
#endif
