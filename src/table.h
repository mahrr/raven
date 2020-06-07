#ifndef raven_table_h
#define raven_table_h

// Simple linear probing hash table

#include "common.h"
#include "value.h"

typedef struct {
    ObjString *key;
    Value value;
} Entry;

typedef struct {
    int count;
    int hash_mask;
    Entry *entries;
} Table;

// Initialize table state.
void init_table(Table *table);

// Dispose table owned memory.
void free_table(Table *table);

// Set value to the value corresponding to key if it's found.
// Return true if a value is found, false otherwise.
bool table_get(Table *table, ObjString *key, Value *value);

// Set the value corresponding to key to value, or add a new
// value if there is no entry for the key.
// Return true if it's a new value, false otherwise.
bool table_set(Table *table, ObjString *key, Value value);

// Remove the value corresponding to key, if it's found.
// Return true if there is a value, false otherwise.
bool table_remove(Table *table, ObjString *key);

// Copy every entry from a table to another.
void table_copy(Table *from, Table *to);

// Return interned string object key, if it's found in
// the table entries, otherwise return NULL.
ObjString *table_interned(Table *table, const char *chars,
                          uint32_t hash, int length);

#endif
