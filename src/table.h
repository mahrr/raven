#ifndef raven_table_h
#define raven_table_h

// Simple linear probing hash table

#include "common.h"
#include "value.h"

typedef struct {
    RavString *key;
    Value value;
} Entry;

typedef struct {
    Entry *entries;
    int count;
    int hash_mask;
} Table;

// Initialize table state.
void table_init(Table *table);

// Dispose table owned memory.
void table_free(Table *table);

// Set value to the value corresponding to key if it's found.
// Return true if a value is found, false otherwise.
bool table_get(Table *table, RavString *key, Value *value);

// Set the value corresponding to key to value, or add a new
// value if there is no entry for the key.
// Return true if it's a new value, false otherwise.
bool table_set(Table *table, RavString *key, Value value);

// Remove the value corresponding to key, if it's found.
// Return the removed value if it existed, or nil otherwise.
Value table_remove(Table *table, RavString *key);

// Copy every entry from a table to another.
void table_copy(Table *from, Table *to);

// Return interned string object key, if it's found in
// the table entries, otherwise return NULL.
RavString *table_interned(Table *table, const char *chars, uint32_t hash, int length);

// Remove the weak referenced strings objects (unreachable white objects).
void table_remove_weak(Table *table);

#endif
