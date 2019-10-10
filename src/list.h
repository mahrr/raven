/*
 * (list.h | 28 Nov 18 | Ahmad Maher)
 *
 * A simple linked list used as the underlying data structure 
 * for the raven list.
 *
*/

#ifndef list_h
#define list_h

typedef struct List {
    void *head;
    struct List *tail;
} List;

/* passed functions types */
typedef void *(*Map_Fn)  (void*);
typedef void *(*Copy_Fn) (void*);
typedef int   (*Pred_Fn) (void*);
typedef void *(*Fold_Fn) (void*, void*);

typedef void (*Iter_Fn) (void*);
typedef void (*Free_Fn) (void*);

/*
 * Note:
 * the function works with NULL as an empty list.
*/

/* 
 * appends two lists, and returns the concatenated list.
 * it's O(m), where m is the length if the first list.
*/
List *list_append(List *list, List *tail);

/*
 * removes an object from the list at specific position
 * (start with 0). it returns the removed object.
*/
void *list_remove(List *list, int pos);

/*
 * adds an object to the end of a list, and retruns it.
*/
List *list_add(List *list, void *x);

/* pushes an object to the beginning of a list. */
List *list_push(List *list, void *x);

/* 
 * pops the first object from the list,
 * puts the object pointer address into x,
 * if x is not NULL, and returns the list. 
*/
List *list_pop(List *list, void **x);

/* reverses a list in place. and returns it. */
List *list_rev(List *list);

/*
 * maps every object in the list using f function.
 * and returns the result list.
 *
*/
List *list_map(List *list, Map_Fn f);

/* 
 * copies every object in the list using f fucntion
 * to a new list. and returns the new list.
*/
List *list_copy(List *list, Copy_Fn f);

/*
 * returns a new list that copies (by cpy) all 
 * objects of the list that satisfy the Predicate f.
*/
List *list_filter(List *list, Pred_Fn f, Copy_Fn cpy);

/*
 * iterates over the list (from left, or right)
 * incrementally appling the function f over the
 * list objects.
 *
 * for list [1, 2, 3]
 * foldl -> f( f( f(b, 1), 2), 3)
 * foldr -> f(1, f(2, f(3, b) ) )
*/
void *list_foldl(List *list, Fold_Fn f, void *b);
void *list_foldr(List *list, Fold_Fn f, void *b);

/* 
 * applies the function f to every object in list. 
*/
void list_iter(List *list, Iter_Fn f);

/*
 * checks if at least one object in the list satisfies
 * the predicate f, if so it returns 1, otherwise 0.
*/
int list_exists(List *list, Pred_Fn f);

/*
 * checks if all objects in the list satisfy the
 * predicate f, if so it returns 1, otherwise 0.
*/
int list_forall(List *list, Pred_Fn f);

/*
 * returns a list of provided objects, with
 * x is the first object. all objects should 
 * be a void pointer and last argument should
 * be a NULL pointer.
 *
*/
List *list_of(void *x, ...);

/* 
 * returns the length of a list.
 * this functions walks the entire list, so
 * it's expensive for long lists.
*/
int list_len(List *list);

/* 
 * convert a list to an continuation vector of the
 * objects pointers. it returns a pointer to the vector.
*/
void **list_to_vec(List *list);

/*
 * free a list pointed by 'lp'.
 * list objects are freed by free_hd function,
 * if provided.
*/
void free_list(List **lp, Free_Fn f);

#endif
