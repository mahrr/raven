#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "compiler.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
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
    vm->objects = NULL;
    
    reset_stack(vm);
}

void free_vm(VM *vm) {
    free_objects(vm->objects);
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
    return Is_Nil(value) || (Is_Bool(value) && !As_Bool(value));
}

static bool equal_values(Value x, Value y) {
    if (x.type != y.type) return false;

    switch (x.type) {
    case VALUE_NUM:  return As_Num(x) == As_Num(y);
    case VALUE_BOOL: return As_Bool(x) == As_Bool(y);
    case VALUE_NIL:  return true;
    case VALUE_OBJ:  return As_Obj(x) == As_Obj(y);
    default:
        assert(0);
    }
}

// The VM main loop.
static InterpretResult run_vm(VM *vm) {
    // Reading Operations
#define Read_Byte()     (*vm->ip++)
#define Read_Constant() (vm->chunk->constants.values[Read_Byte()])
#define Read_Offset()                                       \
    (vm->ip += 2, (uint16_t)(vm->ip[-2] << 8 | vm->ip[-1]))

    // Stack Operations
#define Pop()          (pop(vm))
#define Push(value)    (push(vm, value))
#define Peek(distance) (peek(vm, distance))

    // Arithmetics Binary
#define Binary_OP(value_type, op)                            \
    do {                                                     \
        if (!Is_Num(Peek(0)) || !Is_Num(Peek(1))) {          \
            runtime_error(vm, "Operands must be numbers");   \
            return INTERPRET_RUNTIME_ERROR;                  \
        }                                                    \
                                                             \
        double y = As_Num(Pop());                            \
        double x = As_Num(Pop());                            \
                                                             \
        Push(value_type(x op y));                            \
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
        switch (instruction = Read_Byte()) {
        case OP_LOAD_TRUE:  Push(Bool_Value(true));  break;
        case OP_LOAD_FALSE: Push(Bool_Value(false)); break;
        case OP_LOAD_NIL:   Push(Nil_Value);         break;
        case OP_LOAD_CONST: Push(Read_Constant());   break;
            
        case OP_ADD: Binary_OP(Num_Value, +); break;
        case OP_SUB: Binary_OP(Num_Value, -); break;
        case OP_MUL: Binary_OP(Num_Value, *); break;
        case OP_DIV: Binary_OP(Num_Value, /); break;
        case OP_MOD: /* TODO */ break;
        case OP_NEG: {
            if (!Is_Num(Peek(0))) {
                runtime_error(vm, "Negation operand must be a number");
                return INTERPRET_RUNTIME_ERROR;
            }

            Push(Num_Value(-As_Num(Pop())));
            break;
        }

        case OP_EQ: {
            Value y = Pop();
            Value x = Pop();

            Push(Bool_Value(equal_values(x, y)));
            break;
        }
        case OP_NEQ: {
            Value y = Pop();
            Value x = Pop();

            Push(Bool_Value(!equal_values(x, y)));
            break;
        }
        case OP_LT:  Binary_OP(Bool_Value, <);  break;
        case OP_LTQ: Binary_OP(Bool_Value, <=); break;
        case OP_GT:  Binary_OP(Bool_Value, >);  break;
        case OP_GTQ: Binary_OP(Bool_Value, >=); break;

        case OP_JMP: {
            uint16_t offset = Read_Offset();
            vm->ip += offset;
            break;
        }

        case OP_JMP_FALSE: {
            uint16_t offset = Read_Offset();
            if (is_falsy(Peek(0))) vm->ip += offset;
            break;
        }

        case OP_POP: Pop(); break;
        case OP_NOT: Push(Bool_Value(is_falsy(Pop()))); break;

        case OP_RETURN:
            print_value(Pop());
            putchar('\n');
            return INTERPRET_OK;

        default:
            assert(0);
        }
    }

#undef Peek
#undef Pop
#undef Push
#undef Read_Offset
#undef Read_Constant
#undef Read_Byte
}

InterpretResult interpret(VM *vm, const char *source) {
    Chunk chunk;
    init_chunk(&chunk);
    vm->chunk = &chunk;

    if (!compile(vm, source)) {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    
    vm->ip = chunk.opcodes;
    InterpretResult result = run_vm(vm);

    free_chunk(&chunk);
    return result;
}
