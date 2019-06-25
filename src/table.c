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

/* return the table element with specified key */
Elem *get_elem(Table *table, const void *key) {
    uint64_t hash = table->hash(key);
    unsigned index = hash % table->size;

    Elem *elem;
    List *cell = table->entries[index];
    for ( ; cell; cell = cell->tail) {
        elem = (Elem*)cell->head;
        /* hash comparsion is done first, as it's likely faster
           than key comparsion, especially if the keys are strings. */
        if (hash == elem->hash && table->comp(elem->key, key))
            return elem;
    }

    return NULL;
}

void init_table(Table *table, int size, Hash_Fn hash,
                Free_Fn free, Comp_Fn comp) {
    table->elems = 0;
    table->size = size;
    table->hash = hash;
    table->free = free;
    table->comp = comp;

    /* allocate array of list pointers */
    table->entries = (List**)malloc(sizeof(List*) * size);

    /* allocate lists for each entry */
    for (int i = 0; i < size; i++)
        table->entries[i] = NULL;
}

int table_lookup(Table *table, const void *key) {
    if (get_elem(table, key) != NULL)
        return 1;

    /* element not found */
    return 0;
}

void *table_put(Table *table, const void *key, void *data) {
    /* check if an element with the same key exists */
    Elem *elem = get_elem(table, key);
    void *prev;

    if (elem != NULL) {
        prev = elem->data;  /* the old associated element */
        elem->data = data;  /* put the new element */
        return prev;        /* return the old element */
    }

    /* no element with the same key exists */
    uint64_t hash = table->hash(key);
    unsigned index = hash % table->size;

    /* allocate a new element block */
    elem = malloc(sizeof(Elem));
    elem->key = key;
    elem->hash = hash;
    elem->data = data;
    
    table->entries[index] = list_add(table->entries[index], elem);
    table->elems++;

    return NULL;
}

void *table_get(Table *table, const void *key) {
    Elem *elem = get_elem(table, key);
    return elem ? elem->data : NULL;
}

void *table_remove(Table *table, const void *key) {
    uint64_t hash = table->hash(key);
    unsigned index = hash % table->size;

    Elem *elem;
    List *cell = table->entries[index];
    for (int i = 0; cell; cell = cell->tail, i++) {
        elem = (Elem*)cell->head;
        if (hash == elem->hash && table->comp(elem->key, key)) {
            table->elems--;
            return list_remove(cell, i);
        }
    }

    /* elemnt not found */
    return NULL;
}

void free_table(Table *table) {
    /* deallocating the table elements */
    for (int i = 0; i < table->size; i++) {
        List *cell = table->entries[i];
        List *next;
        Elem *elem;
        for ( ; cell; cell = next) {
            elem = cell->head;
            next = cell->tail;
            table->free(elem->data);
            free(elem);
            free(cell);
        }
    }
    free(table->entries);
}
