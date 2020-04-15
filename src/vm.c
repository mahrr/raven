#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "compiler.h"
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

static inline Value pop(VM *vm) {
    return *--vm->stack_top;
}

static inline Value peek(VM *vm, int distance) {
    return vm->stack_top[-1 - distance];
}

static inline void push(VM *vm, Value value) {
    *vm->stack_top++ = value;
}

static inline bool is_falsy(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static bool equal_values(Value x, Value y) {
    if (x.type != y.type) return false;

    switch (x.type) {
    case VALUE_NUM:  return AS_NUM(x) == AS_NUM(y);
    case VALUE_BOOL: return AS_BOOL(x) == AS_BOOL(y);
    case VALUE_NIL:  return true;
    default:
        assert(0);
    }
}

// The VM main loop.
static InterpretResult run_vm(VM *vm) {
    // Reading Operations
#define READ_BYTE()     (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])

    // Stack Operations
#define POP()          (pop(vm))
#define PUSH(value)    (push(vm, value))
#define PEEK(distance) (peek(vm, distance))

    // Arithmetics Binary
#define BINARY_OP(value_type, op)                            \
    do {                                                     \
        if (!IS_NUM(PEEK(0)) || !IS_NUM(PEEK(1))) {          \
            runtime_error(vm, "Operands must be numbers");   \
            return INTERPRET_RUNTIME_ERROR;                  \
        }                                                    \
                                                             \
        double y = AS_NUM(POP());                            \
        double x = AS_NUM(POP());                            \
                                                             \
        PUSH(value_type(x op y));                            \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value *value = vm->stack; value < vm->stack_top; value++) {
            printf("[ ");
            print_value(*value);
            printf(" ]");
        }
        putchar('\n');
        
        int offset = (int) (vm->ip - vm->chunk->opcodes);
        disassemble_instruction(vm->chunk, offset);
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {            
        case OP_LOAD_TRUE:  PUSH(BOOL_VALUE(true));  break;
        case OP_LOAD_FALSE: PUSH(BOOL_VALUE(false)); break;
        case OP_LOAD_NIL:   PUSH(NIL_VALUE);         break;
        case OP_LOAD_CONST: PUSH(READ_CONSTANT());   break;
            
        case OP_ADD: BINARY_OP(NUM_VALUE, +); break;
        case OP_SUB: BINARY_OP(NUM_VALUE, -); break;
        case OP_MUL: BINARY_OP(NUM_VALUE, *); break;
        case OP_DIV: BINARY_OP(NUM_VALUE, /); break;
        case OP_MOD: /* TODO */ break;
        case OP_NEG: {
            if (!IS_NUM(PEEK(0))) {
                runtime_error(vm, "Negation operand must be a number");
                return INTERPRET_RUNTIME_ERROR;
            }

            PUSH(NUM_VALUE(-AS_NUM(POP())));
            break;
        }

        case OP_EQ: {
            Value y = POP();
            Value x = POP();

            PUSH(BOOL_VALUE(equal_values(x, y)));
            break;
        }
        case OP_NEQ: {
            Value y = POP();
            Value x = POP();

            PUSH(BOOL_VALUE(!equal_values(x, y)));
            break;
        }
        case OP_LT:  BINARY_OP(BOOL_VALUE, <);  break;
        case OP_LTQ: BINARY_OP(BOOL_VALUE, <=); break;
        case OP_GT:  BINARY_OP(BOOL_VALUE, >);  break;
        case OP_GTQ: BINARY_OP(BOOL_VALUE, >=); break;

        case OP_NOT: PUSH(BOOL_VALUE(is_falsy(POP()))); break;

        case OP_RETURN:
            print_value(POP());
            putchar('\n');
            return INTERPRET_OK;

        default:
            assert(0);
        }
    }

#undef PEEK
#undef POP
#undef PUSH
#undef READ_CONSTANT
#undef READ_BYTE
}

InterpretResult interpret(VM *vm, const char *source) {
    Chunk chunk;
    init_chunk(&chunk);
    vm->chunk = &chunk;

    if (!compile(vm, source)) {
        return INTERPRET_COMPILE_ERROR;
    }
    
    vm->ip = chunk.opcodes;
    InterpretResult result = run_vm(vm);

    free_chunk(&chunk);
    return result;
}
