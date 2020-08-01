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
    if (!allocator->gc_off && new_size > old_size) {
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

static void mark_object(Allocator *allocator, Object *object) {
    if (object == NULL) return;
    if (object->marked) return;

#ifdef DEBUG_TRACE_MEMORY
    printf("[Memory] %p get marked: ", object);
    print_value(Obj_Value(object));
    putchar('\n');
#endif

    object->marked = true;

    // No need to be a gray object, if it's a string object.
    if (object->type == OBJ_STRING) return;

    // Add the object to the marked stack.
    if (allocator->gray_count == allocator->gray_capacity) {
        int new_capacity = Grow_Capacity(allocator->gray_capacity);
        size_t size = sizeof (Object*) * new_capacity;
 
        allocator->gray_stack = realloc(allocator->gray_stack, size);
        allocator->gray_capacity = new_capacity;
    }

    allocator->gray_stack[allocator->gray_count++] = object;
}

static void mark_value(Allocator *allocator, Value value) {
    if (!Is_Obj(value)) return;
    mark_object(allocator, As_Obj(value));
}

static void mark_array(Allocator *allocator, Value *values, size_t size) {
    for (size_t i = 0; i < size; i++) {
        mark_value(allocator, values[i]);
    }
}

void mark_roots(Allocator *allocator) {
    VM *vm = (VM *)allocator;

    // Locals and Temporaries
    for (Value *slot = vm->stack; slot < vm->stack_top; slot++) {
        mark_value(allocator, *slot);
    }

    // Call Stack
    for (int i = 0; i < vm->frame_count; i++) {
        mark_object(allocator, (Object *)vm->frames[i].closure);
    }

    // Globals
    for (int i = 0; i < vm->globals.count; i++) {
        mark_value(allocator, vm->global_buffer[i]);
    }

    // Upvalues
    for (RavUpvalue *upvalue = vm->open_upvalues; upvalue != NULL; ) {
        mark_object(allocator, (Object *)upvalue);
        upvalue = upvalue->next;
    }
}

static void blacken_object(Allocator *allocator, Object *object) {
#ifdef DEUBG_TRACE_MEMORY
    printf("[Memory] %p get blacken: ", object);
    print_value(Obj_Value(object));
    putchar('\n');
#endif

    switch (object->type) {
    case OBJ_PAIR: {
        RavPair *pair = (RavPair *)object;
        mark_value(allocator, pair->head);
        mark_value(allocator, pair->tail);
        break;
    }

    case OBJ_ARRAY: {
        RavArray *array = (RavArray *)object;
        mark_array(allocator, array->values, array->count);
        break;
    }

    case OBJ_FUNCTION: {
        RavFunction *function = (RavFunction *)object;
        Chunk *chunk = &function->chunk;

        mark_array(allocator, chunk->constants, chunk->constants_count);
        mark_object(allocator, (Object *)function->name);

        break;
    }

    case OBJ_UPVALUE:
        mark_value(allocator, ((RavUpvalue *)object)->captured);
        break;

    case OBJ_CLOSURE: {
        RavClosure *closure = (RavClosure *)object;
        mark_object(allocator, (Object *)closure->function);

        for (int i = 0; i < closure->upvalue_count; i++) {
            mark_object(allocator, (Object *)closure->upvalues[i]);
        }
        
        break;
    }

    default:
        assert(!"invalid object type");
    }
}

static void trace_references(Allocator *allocator) {
    while (allocator->gray_count > 0) {
        Object *object = allocator->gray_stack[--allocator->gray_count];
        blacken_object(allocator, object);
    }
}

void run_gc(Allocator *allocator) {
#ifdef DEBUG_TRACE_MEMORY
    puts("[Memory] --- GC Round Start ---");
#endif

    mark_roots(allocator);
    trace_references(allocator);
    
#ifdef DEBUG_TRACE_MEMORY
    puts("[Memory] --- GC Round End ---");
#endif
}

void init_allocator(Allocator *allocator) {
    allocator->objects = NULL;
    allocator->gray_stack = NULL;
    allocator->gray_count = 0;
    allocator->gray_capacity = 0;
    allocator->gc_off = false;
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
    free(allocator->gray_stack);
    
    Object *objects = allocator->objects;
    while (objects) {
        Object *next = objects->next;
        free_object(allocator, objects);
        objects = next;
    }

    init_allocator(allocator);
}
