#include <stdlib.h>

#include "mem.h"
#include "object.h"
#include "table.h"

#ifdef DEBUG_TRACE_MEMORY
#include <stdio.h>
#include "debug.h"
#endif

void *allocate(Allocator *allocator, void *previous, size_t old_size,
               size_t new_size) {
    if (allocator != NULL && new_size > old_size) {
#ifdef DEBUG_STRESS_GC
        run_gc(allocator);
#endif
    }
    
    if (new_size == 0) {
        free(previous);
        return NULL;
    }

    return realloc(previous, new_size);
}

void mark_roots(Allocator *allocator) {
    (void)allocator;
}

void run_gc(Allocator *allocator) {
#ifdef DEBUG_TRACE_MEMORY
    puts("[Memory] --- GC Round Start ---");
#endif

    mark_roots(allocator);
    
#ifdef DEBUG_TRACE_MEMORY
    puts("[Memory] --- GC Round End ---");
#endif
}

void init_allocator(Allocator *allocator) {
    allocator->objects = NULL;
    init_table(&allocator->strings);
}

static void free_object(Allocator *allocator, Object *object) {
#ifdef DEUBG_TRACE_MEMORY
    printf("[Memory] %p : free type %d\n", object, object->type);
#endif

    switch (object->type) {
    case OBJ_STRING: {
        RavString *string = (RavString *)object;
        Free_Array(allocator, char, string->chars, string->length + 1);
        Free(allocator, RavString, string);
        break;
    }

    case OBJ_PAIR: {
        Free(allocator, RavPair, object);
        break;
    }

    case OBJ_ARRAY: {
        RavArray *array = (RavArray *)object;
        Free_Array(allocator, Value, array->values, array->capacity);
        Free(allocator, RavArray, array);
        break;
    }

    case OBJ_FUNCTION: {
        RavFunction *function = (RavFunction *)object;
        // TODO: manually free the name or leave it to the GC?
        free_chunk(&function->chunk);
        Free(allocator, RavFunction, function);
        break;
    }

    case OBJ_UPVALUE: {
        Free(allocator, RavUpvalue, object);
        break;
    }

    case OBJ_CLOSURE: {
        RavClosure *closure = (RavClosure *)object;
        Free_Array(allocator,
                   RavClosure*,
                   closure->upvalues,
                   closure->upvalue_count);
        Free(allocator, RavClosure, object);
        break;
    }

    default:
        assert(!"invalid object type");
    }    
}

void free_allocator(Allocator *allocator) {
    free_table(&allocator->strings);
    
    Object *objects = allocator->objects;
    while (objects) {
        Object *next = objects->next;
        free_object(allocator, objects);
        objects = next;
    }
}
