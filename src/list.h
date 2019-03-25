/*
 * (list.h | 28 Nov 18 | Ahmad Maher)
 *
 * simple linked list.
 *
*/

#include "alloc.h"

#ifndef list_h
#define list_h

typedef struct List_T *List_T;
#define T List_T

/* 
   NOTE:
   None of these function that expect a list as an argument
   can be passed NULL List. 
*/

/* return a new empty list allocated on reg region. */
extern T List_new(Region_N reg);

/* appends a list l with object obj. */
extern T List_append(T l, void *obj);

/* iterate over a list l. return the next object or NULL on finish.
   doesn't support nested iteration for the same list 
   (i.e. the nested iteration will continue from the outer iteration).*/
extern void *List_iter(T l);

/* unwind the list iteration to the beginning of the list. */
extern void List_unwind(T l);

/* returns the lenght of a list l. */
extern long List_len(T l);

/* convert a list to an continuation vector allocated on
   region reg with terminated null. */
extern void *List_to_vec(T l, Region_N reg);

#undef T
#endif
