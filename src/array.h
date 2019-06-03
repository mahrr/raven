/*
 * (array.h | 10 May 19 | Ahmad Maher)
 *
 * Simple dynamic arrays macros to avoid redundant
 * type declaration and array operations.
 *
*/

/* 
 * array type on the fly
 * cap: the actual length of the array.
 * len: the logical length of the array.
 * size: the size of each element in the array.
 * elems: pointer to the acutal internal array.
 *
*/
#define ARRAY(type)                             \
    struct {                                    \
        int cap;                                \
        int len;                                \
        size_t size;                            \
        type *elems;                            \
    }

/* array allocation and initialization */
#define ARR_INIT(arr, type)                     \
    arr = malloc(sizeof(*arr));                 \
    arr->len = 0;                               \
    arr->cap = 8;                               \
    arr->size = sizeof(type);                   \
    arr->elems = malloc(8 * arr->size)

/* add an element */
#define ARR_ADD(arr, obj)                                       \
    if (arr->len < arr->cap)                                    \
        arr->elems[len++] = obj;                                \
    else {                                                      \
        arr->cap *= 2;                                          \
        arr->elems = realloc(arr->elems, arr->cap * arr->size); \
        arr->elems[len++] = obj;                                \
    }

/* dispose the array elements, and the array struct */
#define ARR_FREE(arr)                           \
    free(arr->elems);                           \
    free(arr);                                  \
