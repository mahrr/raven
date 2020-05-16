#ifndef raven_vm_h
#define raven_vm_h

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

// The limit of nested frames.
#define FRAME_LIMIT 128

// Maximum number of values on the stack.
#define STACK_SIZE (256 * FRAME_LIMIT)

typedef struct {
    Value stack[STACK_SIZE];
    Value *stack_top;
    
    Chunk *chunk;
    uint8_t *ip;

    Table strings;    // Interned strings (All strings on the system).
    Table globals;
    
    Object *objects;  // Intrusive linked list of all allocated objects.
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

// Initialize the vm to an empty state.
void init_vm(VM *vm);

// Free the resources owned by the vm.
void free_vm(VM *vm);

// Execute the given source code, and return
// the interpretation result.
InterpretResult interpret(VM *vm, const char *source);

#endif
