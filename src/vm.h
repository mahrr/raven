#ifndef raven_vm_h
#define raven_vm_h

#include "common.h"
#include "chunk.h"
#include "mem.h"
#include "table.h"
#include "value.h"

typedef struct {
    RavClosure *closure;
    uint8_t *ip;
    Value *slots;
} CallFrame;

// Virtual Machine Image
typedef struct {
    Allocator allocator;
    const char *path; // Name of the file being executed.

    Value x;  // Register to store last evaluated expression.
    Value stack[STACK_SIZE];
    Value *stack_top;

    bool reset_on_exit; // whether or not to reset the stack on OP_EXIT

    CallFrame frames[FRAMES_LIMIT];
    int frame_count;

    // Map each used global name to its index at the global variables
    // buffer. It's populated at compile time, as the parser resolve
    // the identifiers.
    Table globals;

    // Used to obtain globals variables at runtime.
    Value global_buffer[GLOBALS_LIMIT];

    // Intrusive linked list of all available open opvalues.
    // TODO: experiment with using a hash table instead.
    RavUpvalue *open_upvalues;
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
InterpretResult interpret(VM *vm, const char *source, const char *path);

#endif
