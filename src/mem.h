#ifndef raven_mem_h
#define raven_mem_h

#include "value.h"
#include "table.h"

// Raven Objects Allocator
typedef struct {
    Table strings;   // Table of all interned strings in a vm image.
    Object *objects; // Intrusive linked list of all allocated objects.
} Allocator;

#define Alloc(allocator, type, size)                                \
    (type *)allocate(allocator, NULL, 0, (size) * sizeof (type))

#define Free(allocator, type, pointer)                      \
    (void)allocate(allocator, (pointer), sizeof (type), 0)

#define Grow_Capacity(capacity)                 \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define Grow_Array(allocator, array, type, old_size, new_size)  \
    (type *)allocate(allocator, (array),                        \
                     sizeof(type) * (old_size),                 \
                     sizeof(type) * (new_size))


#define Free_Array(allocator, type, array, size)                    \
    (void)allocate(allocator, (array), sizeof(type) * (size), 0)

//
// The main entry point for most of the system allocation, freeing
// and reallocation.
//
//  -------------------------------------------------------------------
// |previous   | old_size  | new_size | action                         |
// |-------------------------------------------------------------------|
// | NULL      | 0         | X        | allocate X bytes               |
// | P         | X         | Y        | reallocate P from X to Y bytes |
// | P         | -         | 0        | free P memory                  |
//  -------------------------------------------------------------------
//
void *allocate(Allocator *allocator, void *previous, size_t old_size,
               size_t new_size);

void run_gc(Allocator *allocator);

void init_allocator(Allocator *allocator);

void free_allocator(Allocator *allocator);

#endif
