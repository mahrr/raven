#ifndef raven_mem_h
#define raven_mem_h

#include "value.h"
#include "table.h"

//
// tri-color state of an object:
// -----------------------------
//   white -> not processed yet
//            marked field is false
//
//   gray  -> reachable, marked field is true
//            the gc didn't trace through its references yet
//            present in the gray stack
//
//   black -> reachable, marked field is true
//            the gc traced through its references
//            not present in the gray stack
//

// Raven Objects Allocator
typedef struct {
    // Table of all interned strings in a vm image.
    Table strings;

    // Intrusive linked list of all allocated objects.
    Object *objects;

    // Array of currently marked, but not processed, objects.
    Object **gray_stack;
    int gray_capacity;
    int gray_count;

    // Allocation stats
    size_t bytes_allocated;
    size_t next_gc;

    // Flag to disable the Garbage Collector.
    bool gc_off;
} Allocator;


// The basic variant of the raven GC is as the amount of live objects
// increases, the GC runs less avoiding wasting time re-traversing
// the high number of already live objects (optimize throughput), and
// run more frequently as the amount of live objects decreases to not
// wait too long on dead objects (decrease latency).

// TODO: parameterize these factors with environment variables.

#define GC_INITIAL_NEXT  1048576UL
#define GC_GROWTH_FACTOR 2

#define Alloc(allocator, type, size)                                \
    (type *)allocate(allocator, NULL, 0, (size) * sizeof (type))

#define Free(allocator, type, pointer)                              \
    (void)allocate(allocator, (pointer), sizeof (type), 0)

#define Grow_Capacity(capacity)                                     \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define Grow_Array(allocator, array, type, old_size, new_size)  \
    (type *)allocate(allocator, (array),                        \
                     sizeof(type) * (old_size),                 \
                     sizeof(type) * (new_size))


#define Free_Array(allocator, type, array, size)                    \
    (void)allocate(allocator, (array), sizeof(type) * (size), 0)

void init_allocator(Allocator *allocator);

void free_allocator(Allocator *allocator);

//
// The main entry point for most of the runtime raven objects allocation
// freeing and reallocation.
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

#endif
