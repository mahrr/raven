/*
 * (array.h | 10 May 19 | Ahmad Maher)
 *
 * Simple dynamic arrays macros to avoid redundant
 * type declaration and array operations.
 *
 */

#ifndef array_h
#define array_h

#include <stdlib.h>

/* array type on the fly
 * cap: the actual length of the array.
 * len: the logical length of the array.
 * size: the size of each element in the array.
 * elems: pointer to the acutal internal array.
 */
#define ARRAY(type)                             \
    struct {                                    \
        int cap;                                \
        int len;                                \
        size_t size;                            \
        type *elems;                            \
    }

/* initialize an array with custome capacity 'n' */
#define ARR_INITC(arr, type, n)                 \
    (arr)->len = 0;                             \
    (arr)->cap = n;                             \
    (arr)->size = sizeof(type);                 \
    (arr)->elems = malloc(n * (arr)->size)

/* initialize an array with default capacity value 8 */
#define ARR_INIT(arr, type)                     \
    ARR_INITC(arr, type, 8)

/* add an element 'obj' */
#define ARR_ADD(arr, obj)                                               \
    if ((arr)->len < (arr)->cap)                                        \
        (arr)->elems[(arr)->len++] = obj;                               \
    else {                                                              \
        (arr)->cap *= 2;                                                \
        (arr)->elems = realloc((arr)->elems, (arr)->cap*(arr)->size);   \
        (arr)->elems[(arr)->len++] = obj;                               \
    }

/* dispose the array elements */
#define ARR_FREE(arr)                           \
    free((arr)->elems);

#endif
