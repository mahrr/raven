#include <stdlib.h>

#include "mem.h"

void *allocate(void *previous, size_t old_size, size_t new_size) {
    (void) old_size;  // Temporary for unused parameter warning.
    
    if (new_size == 0) {
        free(previous);
        return NULL;
    }

    return realloc(previous, new_size);
}
