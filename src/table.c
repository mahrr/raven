#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void init_table(Table *table) {
    table->entries = NULL;
    table->count = 0;
    table->hash_mask = -1;
}

void free_table(Table *table) {
    free(table->entries);
    init_table(table);
}

static Entry *find_entry(Entry *entries, RavString *key, int hash_mask) {
    uint32_t index = key->hash & hash_mask;
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

        index = (index + 1) & hash_mask;
    }
}

static void adjust_capacity(Table *table, int hash_mask) {
    Entry *entries = malloc((hash_mask + 1) * sizeof (Entry));

    for (int i = 0; i <= hash_mask; i++) {
        entries[i].key = NULL;
        entries[i].value = Nil_Value;
    }

    table->count = 0;
    for (int i = 0; i <= table->hash_mask; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }

        Entry *copy = find_entry(entries, entry->key, hash_mask);
        copy->key = entry->key;
        copy->value = entry->value;
        table->count++;
    }

    free(table->entries);
    table->entries = entries;
    table->hash_mask = hash_mask;
}

bool table_get(Table *table, RavString *key, Value *value) {
    if (table->count == 0) {
        return false;
    }

    Entry *entry = find_entry(table->entries, key, table->hash_mask);
    if (entry->key == NULL) {
        return false;
    }

    *value = entry->value;
    return true;
}

bool table_set(Table *table, RavString *key, Value value) {
    int capacity = table->hash_mask + 1;

    if (table->count >= capacity * TABLE_MAX_LOAD) {
        int new_capacity = Grow_Capacity(capacity);
        adjust_capacity(table, new_capacity - 1);
    }

    Entry *entry = find_entry(table->entries, key, table->hash_mask);

    bool is_new_key = entry->key == NULL;
    if (is_new_key && Is_Nil(entry->value)) {
        table->count++;
    }

    entry->key = key;
    entry->value = value;
    return is_new_key;
}

Value table_remove(Table *table, RavString *key) {
    if (table->count == 0) {
        return Nil_Value;
    }

    Entry *entry = find_entry(table->entries, key, table->hash_mask);
    if (entry->key == NULL) {
        return Nil_Value;
    }

    Value value = entry->value;
    entry->key = NULL;
    entry->value = Bool_Value(false); // Any non-nil value

    return value;
}

void table_copy(Table *from, Table *to) {
    for (int i = 0; i <= from->hash_mask; i++) {
        Entry *entry = &from->entries[i];

        if (entry->key) {
            table_set(to, entry->key, entry->value);
        }
    }
}

RavString *table_interned(Table *table, const char *chars, uint32_t hash, int length) {
    if (table->count == 0) {
        return NULL;
    }

    uint32_t index = hash & table->hash_mask;
    for (;;) {
        Entry *entry = &table->entries[index];

        if (entry->key == NULL) {
            if (Is_Nil(entry->value)) {
                return NULL;
            }
        } else {
            RavString *key = entry->key;
            if (key->length == length &&
                key->hash == hash &&
                memcmp(key->chars, chars, length) == 0) {
                return key;
            }
        }

        index = (index + 1) & table->hash_mask;
    }
}

void table_remove_weak(Table *table) {
    for (int i = 0; i <= table->hash_mask; i++) {
        Entry *entry = &table->entries[i];

        if (entry->key != NULL && !entry->key->header.marked) {
            table_remove(table, entry->key);
        }
    }
}
