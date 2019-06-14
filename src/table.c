/*
 * (table.c | 9 May 19 | Ahmad Maher)
 *
 * Hash table implementation.
 *
 * It's a chainded table consists of an array of linked lists, 
 * each list hold element blocks of data with a similar hash code.
 *
 * Insertion and retrival is a constant time operation assuming
 * a hash function with an uniformal distribution.
 *
 * Initailzation and deallocation is O(m) where m is the size of
 * the table.
 *
*/

#include <stdlib.h>
#include <stdint.h>

#include "list.h"
#include "table.h"

/* element block */
typedef struct Elem {
    const void *key;
    uint64_t hash;
    void *data;
} Elem;

void init_table(Table *table, int size, Hash_Fn hash,
                Free_Fn free, Comp_Fn comp) {
    table->elems = 0;
    table->size = size;
    table->hash = hash;
    table->free = free;
    table->comp = comp;

    /* allocate array of list pointers */
    table->entries = (List *)malloc(sizeof(List) * size);

    /* allocate lists for each entry */
    for (int i = 0; i < size; i++)
        table->entries[i] = List_new(R_SECN);
}

int table_lookup(Table *table, const void *key) {
    uint64_t hash = table->hash(key);
    unsigned index = hash % table->size;

    Elem *elem;
    while ((elem = List_iter(table->entries[index]))) {
        /* hash comparsion is done first, as it's likely faster
           than key comparsion, especially if the keys are strings. */
        if (hash == elem->hash && table->comp(elem->key, key))
            return 1;
    }

    /* element not found */
    return 0;
}

void *table_put(Table *table, const void *key, void *data) {
    uint64_t hash = table->hash(key);
    unsigned index = hash % table->size;

    Elem *elem;
    void *prev;

    /* check if an element with the same key exists */
    while ((elem = List_iter(table->entries[index]))) {
        if (hash == elem->hash && table->comp(elem->key, key)) {
            prev = elem->data;  /* the old associated element */
            elem->data = data;  /* put the new element */
            return prev;        /* return the old element */
        }
    }

    /* no element with the same key exists */

    /* allocate a new element block */
    elem = malloc(sizeof(Elem));
    elem->key = key;
    elem->hash = hash;
    elem->data = data;
    
    List_append(table->entries[index], elem);
    table->elems++;

    return NULL;
}

void *table_get(Table *table, const void *key) {
    uint64_t hash = table->hash(key);
    unsigned index = hash % table->size;
    
    Elem *elem;
    while ((elem = List_iter(table->entries[index]))) {
        if (hash == elem->hash && table->comp(elem->key, key))
            return elem->data;
    }

    /* element not found */
    return NULL;
}

void *table_remove(Table *table, const void *key) {
    uint64_t hash = table->hash(key);
    unsigned index = hash % table->size;

    Elem *elem;
    while ((elem = List_iter(table->entries[index]))) {
        if (hash == elem->hash && table->comp(elem->key, key)) {
            table->elems--;
            // return List_remove();
        }
    }

    /* elemnt not found */
    return NULL;
}

void table_free(Table *table) {
    /* deallocating the table elements */
    for (int i = 0; i < table->size; i++)
        ;//list_free(table->entries[i], table->free);

    free(table->entries);
}
