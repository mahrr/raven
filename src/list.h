/*
 * (list.h | 28 Nov 18 | Ahmad Maher)
 *
 * linked list implementation suited for the allocation scheme.
 *
*/

#ifndef list_h
#define list_h

typedef struct list {
    void *obj;
    struct list *link;
} list;

/* appends a list l with object obj or creates it if l is 
   NULL. return the new list.*/
extern list *append_list(list *l, void *obj);

/* removes an object from a list if it was found.
   return the new list or Null on failure. */
extern list *remove_obj(list *l, void *obj);

/* iterate over a list l. returns the next object or
   NULL on finish Caution this function works with
   global variables,so it's not thread-safe. */
extern void *iter_list(list *l);

/* returns the lenght of a list l. */
extern long len_list(list *l);

/* convert a list to an continuation vector allocated on
   region reg with terminated null and deallocate the 
   list structures. */
extern void *list_to_vec(list *l, unsigned reg);

#endif
