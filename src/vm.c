#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "vm.h"


#ifdef DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif

static inline void reset_stack(VM *vm) {
    vm->stack_top = vm->stack;
}

void init_vm(VM *vm) {
    vm->chunk = NULL;
    vm->ip = NULL;
    
    reset_stack(vm);
}

void free_vm(VM *vm) {
    init_vm(vm);
}

static void runtime_error(VM *vm, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);

    // -1 obecause ip is sitting on the next instruction to be executed.
    int offset = (int) (vm->ip - vm->chunk->opcodes - 1);
    fprintf(stderr, "[line: %d] ", decode_line(vm->chunk, offset));
    
    vfprintf(stderr, format, arguments);
    putc('\n', stderr);
    
    reset_stack(vm);
}

// The VM main loop.
static InterpretResult run_vm(VM *vm) {
    // Reading Operations
#define READ_BYTE()     (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])

    // Stack Operations
#define POP()          (*--vm->stack_top)
#define PUSH(value)    (*vm->stack_top++ = (value))
#define PEEK(distance) (vm->stack_top[-1 - (distance)])

    // Arithmetics Binary
#define BINARY_OP(op)                                        \
    do {                                                     \
        if (!IS_NUM(PEEK(0)) && !IS_NUM(PEEK(1))) {          \
            runtime_error(vm, "Operands must be numbers");   \
            return INTERPRET_RUNTIME_ERROR;                  \
        }                                                    \
                                                             \
        double x = AS_NUM(POP());                            \
        double y = AS_NUM(POP());                            \
                                                             \
        PUSH(NUM_VALUE(x op y));                             \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value *value = vm->stack; value < vm->stack_top; value++) {
            printf("| ");
            print_value(*value);
            printf(" |");
        }
        
        int offset = (int) (vm->ip - vm->chunk->opcodes);
        disassemble_instruction(vm->chunk, offset);
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
        case OP_TRUE:  PUSH(BOOL_VALUE(true)); break;
        case OP_FALSE: PUSH(BOOL_VALUE(false)); break;
        case OP_NIL:   PUSH(NIL_VALUE); break;

        case OP_LOAD_CONST: {
            Value constant = READ_CONSTANT();
            PUSH(constant);
            break;
        }
            
        case OP_ADD: BINARY_OP(+); break;
        case OP_SUB: BINARY_OP(-); break;
        case OP_MUL: BINARY_OP(*); break;
        case OP_DIV: BINARY_OP(/); break;
        case OP_MOD: /* TODO */ break;

        case OP_RETURN:
            print_value(POP());
            putchar('\n');
            return INTERPRET_OK;

        default:
            assert(0);
        }
    }

#undef PUSH
#undef POP
#undef READ_CONSTANT
#undef READ_BYTE
}

InterpretResult interpret(VM *vm, const char *source) {
    (void) source; // Temporary for warnings
    
    Chunk chunk;
    init_chunk(&chunk);
    
    vm->chunk = &chunk;
    vm->ip = chunk.opcodes;

    InterpretResult result = run_vm(vm);

    free_chunk(&chunk);
    return result;
}
