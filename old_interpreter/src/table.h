/*
 * (table.h | 9 May 19 | Ahmad Maher)
 *
 * Chained hash table used as the underlying data structure
 * for raven hash object, and used through out the evaluator
 * internals.
 *
*/


#ifndef table_h
#define table_h

#include <stdint.h>

#include "array.h"

/* function types */
typedef uint64_t (*Hash_Fn)  (const void *key);
typedef void     (*DFree_Fn) (void *data);
typedef int      (*Comp_Fn)  (const void *key1, const void *key2);

/* element block */
typedef struct Elem {
    const void *key;
    uint64_t hash;
    void *data;
} Elem;

typedef struct Entry {
    struct Elem *elem;
    struct Entry *link;
} Entry;

typedef struct Table {
    int elems;              /* number of elements in the table */
    int size;               /* number of entries in the table */
    Hash_Fn hash;           /* key hashing function */
    Comp_Fn comp;           /* key comparing function */
    DFree_Fn free;          /* element deallocation function */
    Entry **entries;        /* array of linked lists of elements blocks */
    ARRAY(int) indexes;     /* array of indexes of populated entries */
} Table;

#define table_size(table) ((table)->size)   /* get number of entries */
#define table_elems(table) ((table)->elems) /* get number of elements */

/*
 * Initialize a table. set its attributes and allocate
 * the table internal array.
 * 
 * table: a pointer to the table to be initialized
 * size: number of the entries expected in the table.
 * hash: function used for key hashing.
 * free: function used for table elements data deallocation.
 * comp: function used for comparing two keys.
 *       it returns 1 if the two keys are equal, 0 otherwise.
*/
void init_table(Table *table, int size, Hash_Fn hash,
                DFree_Fn free, Comp_Fn comp);

/*
 * Check if the key is already exist in the table. if so,
 * it returns 1, otherwise 0.
*/
int table_lookup(Table *table, const void *key);

/*
 * Insert a new element into the table. if the key is
 * already exist, it returns the previous value associated
 * with the key, otherwise it returns NULL.
*/
void *table_put(Table *table, const void *key, void *data);

/*
 * Get the element associated with a key. if the key is
 * found, it returns the element, otherwise it returns NULL.
 * Note that the function is ambiguous regarding the return
 * element is NULL or it's not found. table_lookup is a
 * better alternative for checking.
*/
void *table_get(Table *table, const void *key);

/*
 * Remove an element from the table. if the key is
 * found, it removes the associated element from the
 * table and return it, otherwise it does nothing 
 * and return NULL.
*/
void *table_remove(Table *table, const void *key);

/*
 * Deallocate the table internal array, and call the table free 
 * function for each table element, if it's provided.
*/
void free_table(Table *table);

#endif
