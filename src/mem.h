#ifndef raven_mem_h
#define raven_mem_h

#define ALLOC(type, size)                               \
    (type *)allocate(NULL, 0, (size) * sizeof (type))

#define FREE(type, pointer)                     \
    (void)allocate((pointer), sizeof (type), 0)

#define GROW_CAPACITY(capacity)                 \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(array, type, old_size, new_size)     \
    (type *)allocate((array),                           \
                     sizeof(type) * (old_size),         \
                     sizeof(type) * (new_size))


#define FREE_ARRAY(array, type, size)                   \
    (void)allocate((array), sizeof(type) * (size), 0)

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
void *allocate(void *previous, size_t old_size, size_t new_size);

#endif
