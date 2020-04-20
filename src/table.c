#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "object.h"
#include "mem.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void init_table(Table *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_table(Table *table) {
    Free_Array(Entry, table->entries, table->capacity);
    init_table(table);
}

static Entry *find_entry(Entry *entries, ObjString *key, int capacity) {
    uint32_t index = key->hash % capacity;
    Entry *tombstone = NULL;

    for (;;) {
        Entry *entry = &entries[index];

        if (entry->key == NULL) {
            if (Is_Nil(entry->value)) {
                return tombstone ? tombstone : entry;
            }

            if (tombstone == NULL) tombstone = entry;
        } else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(Table *table, int capacity) {
    Entry *entries = Alloc(Entry, capacity);

    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = Nil_Value;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) continue;

        Entry *copy = find_entry(entries, entry->key, capacity);
        copy->key = entry->key;
        copy->value = entry->value;
        table->count++;
    }

    Free_Array(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool table_get(Table *table, ObjString *key, Value *value) {
    if (table->count == 0) return false;

    Entry *entry = find_entry(table->entries, key, table->capacity);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

bool table_set(Table *table, ObjString *key, Value value) {
    if (table->count >= table->capacity * TABLE_MAX_LOAD) {
        int new_capacity = Grow_Capacity(table->capacity);
        adjust_capacity(table, new_capacity);
    }

    Entry *entry = find_entry(table->entries, key, table->capacity);

    bool is_new_key = entry->key == NULL;
    if (is_new_key && Is_Nil(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return is_new_key;
}

bool table_remove(Table *table, ObjString *key) {
    if (table->count == 0) return false;

    Entry *entry = find_entry(table->entries, key, table->capacity);
    if (entry->key == NULL) return false;

    entry->key = NULL;
    entry->value = Bool_Value(false); // Any non-nil value

    return true;
}

void table_copy(Table *from, Table *to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry *entry = &from->entries[i];

        if (entry->key) {
            table_set(to, entry->key, entry->value);
        }
    }
}

ObjString *table_interned(Table *table, const char *chars,
                          uint32_t hash, int length) {
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry *entry = &table->entries[index];

        if (entry->key == NULL && Is_Nil(entry->value)) return NULL;

        ObjString *key = entry->key;

        if (key->length == length &&
            key->hash == hash &&
            memcmp(key->chars, chars, length) == 0) {
            return key;
        }

        index = (index + 1) % table->capacity;
    }
}
