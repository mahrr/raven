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
    vm->x = Nil_Value;
    vm->frame_count = 0;
    vm->objects = NULL;
    
    init_table(&vm->globals);
    init_table(&vm->strings);
    reset_stack(vm);
}

void free_vm(VM *vm) {
    free_table(&vm->globals);
    free_table(&vm->strings);
    free_objects(vm->objects);

    init_vm(vm);
}

static void dump_stack_trace(VM *vm, FILE *out) {
    fprintf(out, "stack traceback:\n");

    // TODO: an option to control the stack trace dumping order.
    for (int i = vm->frame_count - 1; i >= 0; i--) {
        CallFrame *frame = &vm->frames[i];
        ObjFunction *function = frame->function;

        size_t offset = frame->ip - frame->function->chunk.opcodes - 1;
        int line = decode_line(&frame->function->chunk, offset);
        
        fprintf(out, "\t%s | line:%d in ", vm->path, line);

        if (function->name == NULL) {
            fprintf(out, "<toplevel>\n");
        } else {
            fprintf(out, "'%s'\n", function->name->chars);
        }
    }
}

static void runtime_error(VM *vm, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);

    CallFrame *frame = &vm->frames[vm->frame_count - 1];

    // -1 because ip is sitting on the next instruction to be executed.
    size_t offset = frame->ip - frame->function->chunk.opcodes - 1;
    int line = decode_line(&frame->function->chunk, offset);
    fprintf(stderr, "[%s | line: %d] ", vm->path, line);
    
    vfprintf(stderr, format, arguments);
    putc('\n', stderr);

    dump_stack_trace(vm, stderr);

    va_end(arguments);
    reset_stack(vm);
}

static inline Value pop(register VM *vm) {
    return *--vm->stack_top;
}

static inline Value peek(register VM *vm, int distance) {
    return vm->stack_top[-1 - distance];
}

static inline void push(register VM *vm, Value value) {
    *vm->stack_top++ = value;
}

static inline bool is_falsy(Value value) {
    return Is_Nil(value) || (Is_Bool(value) && !As_Bool(value));
}

static inline bool push_frame(VM *vm, ObjFunction *function, int count) {
    if (vm->frame_count == FRAMES_LIMIT) {
        runtime_error(vm, "call stack overflows");
        return false;
    }
    
    CallFrame *frame = &vm->frames[vm->frame_count++];
    frame->function = function;
    frame->ip = function->chunk.opcodes;
    frame->slots = vm->stack_top - count - 1;

    return true;
}

static inline bool call_func(VM *vm, ObjFunction *function, int count) {
    if (function->arity != count) {
        runtime_error(vm, "expect %d arguments, but got %d",
                      function->arity, count);
        return false;
    }

    return push_frame(vm, function, count);
}

static inline bool call_value(VM *vm, Value value, int count) {
    if (Is_Function(value)) {
        return call_func(vm, As_Function(value), count);
    }

    runtime_error(vm, "call to a non-callable");
    return false;
}

// Returns the name of a registered global at a given index.
// It's a linear function, but that is not a problem, since it
// only gets called at runtime errors.
static inline const char *global_name_at(VM *vm, uint8_t index) {
    Table *globals = &vm->globals;
    
    for (int i = 0; i <= globals->hash_mask; i++) {
        if (globals->entries[i].key == NULL) continue;

        if ((uint8_t)As_Num(globals->entries[i].value) == index) {
            return globals->entries[i].key->chars;
        }
    }

    assert(!"Query the name of unregistered global variable");
    return NULL;
}

// VM Dispatch Loop
static InterpretResult run_vm(register VM *vm) {
    register CallFrame frame = vm->frames[vm->frame_count - 1];

#ifdef DEBUG_TRACE_EXECUTION
#define Log_Execution()                                                  \
    do {                                                                 \
        printf("          ");                                            \
        for (Value *value = vm->stack; value < vm->stack_top; value++) { \
            printf("[ ");                                                \
            print_value(*value);                                         \
            printf(" ]");                                                \
        }                                                                \
        printf(" X: ");                                                  \
        print_value(vm->x);                                              \
        putchar('\n');                                                   \
                                                                         \
        int offset = (int) (frame.ip - frame.function->chunk.opcodes);   \
        disassemble_instruction(&frame.function->chunk, offset);         \
    } while (false)
#else
#define Log_Execution()
#endif  // DEBUG_TRACE_EXECUTION
    
#ifdef THREADED_CODE
    
    static void *dispatch_table[] = {
#define Opcode(opcode) &&label_##opcode,
# include "opcode.h"
#undef Opcode
    };

#define Start() Dispatch();    
#define Case(opcode) label_##opcode
#define Dispatch()                                  \
    Log_Execution();                                \
    goto *dispatch_table[instruction = Read_Byte()]
    
#else

#define Start()                                 \
    vm_loop:                                    \
        switch (instruction = Read_Byte())
#define Case(opcode) case opcode
#define Dispatch()                              \
    Log_Execution();                            \
    goto vm_loop
    
#endif // THREADED_CODE

    // Reading Operations
#define Read_Byte() (*frame.ip++)
#define Read_Offset()                                                   \
    (frame.ip += 2, (uint16_t)(frame.ip[-2] << 8 | frame.ip[-1]))
#define Read_Constant()                                                 \
    (frame.function->chunk.constants[Read_Byte()])
#define Read_String() (As_String(Read_Constant()))

    // Stack Operations
#define Pop()          (pop(vm))
#define Push(value)    (push(vm, value))
#define Peek(distance) (peek(vm, distance))

    // Register the current cached frame
#define Save_Frame() vm->frames[vm->frame_count - 1] = frame
#define Runtime_Error(fmt, ...)                 \
    do {                                        \
        Save_Frame();                           \
        runtime_error(vm, fmt, ##__VA_ARGS__);  \
    } while (false)
    
    // Arithmetics Binary
#define Binary_OP(value_type, op)                            \
    do {                                                     \
        if (!Is_Num(Peek(0)) || !Is_Num(Peek(1))) {          \
            Runtime_Error("Operands must be numbers");       \
            return INTERPRET_RUNTIME_ERROR;                  \
        }                                                    \
                                                             \
        double y = As_Num(Pop());                            \
        double x = As_Num(Pop());                            \
                                                             \
        Push(value_type(x op y));                            \
    } while (false)
    
    register uint8_t instruction;
    Start() {
      Case(OP_PUSH_TRUE):  Push(Bool_Value(true));  Dispatch();
      Case(OP_PUSH_FALSE): Push(Bool_Value(false)); Dispatch();
      Case(OP_PUSH_NIL):   Push(Nil_Value);         Dispatch();
      Case(OP_PUSH_CONST): Push(Read_Constant());   Dispatch();

      Case(OP_PUSH_X):
        Push(vm->x);
        vm->x = Nil_Value;
        Dispatch();
            
      Case(OP_SAVE_X):
        vm->x = Pop();
        Dispatch();

      Case(OP_POP): Pop(); Dispatch();
      Case(OP_POPN): {
            uint8_t count = Read_Byte();
            vm->stack_top -= count;
            Dispatch();
        }
                        
      Case(OP_ADD): Binary_OP(Num_Value, +); Dispatch();
      Case(OP_SUB): Binary_OP(Num_Value, -); Dispatch();
      Case(OP_MUL): Binary_OP(Num_Value, *); Dispatch();
      Case(OP_DIV): Binary_OP(Num_Value, /); Dispatch();
      Case(OP_MOD): /* TODO */ Dispatch();
      Case(OP_NEG): {
            if (!Is_Num(Peek(0))) {
                Runtime_Error("Negation operand must be numeric");
                return INTERPRET_RUNTIME_ERROR;
            }
            Push(Num_Value(-As_Num(Pop())));
            Dispatch();
        }

      Case(OP_EQ): {
            Value y = Pop();
            Value x = Pop();
                
            Push(Bool_Value(equal_values(x, y)));
            Dispatch();
        }
            
      Case(OP_NEQ): {
            Value y = Pop();
            Value x = Pop();
                
            Push(Bool_Value(!equal_values(x, y)));
            Dispatch();
        }
            
      Case(OP_LT):  Binary_OP(Bool_Value, <);  Dispatch();
      Case(OP_LTQ): Binary_OP(Bool_Value, <=); Dispatch();
      Case(OP_GT):  Binary_OP(Bool_Value, >);  Dispatch();
      Case(OP_GTQ): Binary_OP(Bool_Value, >=); Dispatch();
        
      Case(OP_NOT): Push(Bool_Value(is_falsy(Pop()))); Dispatch();
        
      Case(OP_DEF_GLOBAL): {
            vm->global_buffer[Read_Byte()] = Pop();
            Dispatch();
        }
            
      Case(OP_SET_GLOBAL): {
            uint8_t index = Read_Byte();
            
            if (Is_Void(vm->global_buffer[index])) {
                Runtime_Error("unbound variable '%s'",
                              global_name_at(vm, index));
                return INTERPRET_RUNTIME_ERROR;
            }
            
            vm->global_buffer[Read_Byte()] = Peek(0);
            Dispatch();
        }
            
      Case(OP_GET_GLOBAL): {
            uint8_t index = Read_Byte();
            Value value = vm->global_buffer[index];
            
            if (Is_Void(value)) {
                Runtime_Error("unbound variable '%s'",
                              global_name_at(vm, index));
                return INTERPRET_RUNTIME_ERROR;
            }
            
            Push(value);
            Dispatch();
        }

      Case(OP_SET_LOCAL): {
            uint8_t slot = Read_Byte();
            frame.slots[slot] = Peek(0);
            Dispatch();
        }
            
      Case(OP_GET_LOCAL): {
            uint8_t slot = Read_Byte();
            Push(frame.slots[slot]);
            Dispatch();
        }
            
      Case(OP_CALL): {
            int argument_count = Read_Byte();
            Value value = Peek(argument_count);

            Save_Frame();
            if (!call_value(vm, value, argument_count)) {
                return INTERPRET_RUNTIME_ERROR;
            }
                
            // Push the callee new call frame.
            frame = vm->frames[vm->frame_count - 1];
                
            Dispatch();
        }

      Case(OP_JMP): {
            uint16_t offset = Read_Offset();
            frame.ip += offset;
            Dispatch();
        }

      Case(OP_JMP_BACK): {
            uint16_t offset = Read_Offset();
            frame.ip -= offset;
            Dispatch();
        }

      Case(OP_JMP_FALSE): {
            uint16_t offset = Read_Offset();
            if (is_falsy(Peek(0))) frame.ip += offset;
            Dispatch();
        }

      Case(OP_JMP_POP_FALSE): {
            uint16_t offset = Read_Offset();
            if (is_falsy(Pop())) frame.ip += offset;
            Dispatch();
        }

      Case(OP_RETURN): {
            Value result = Pop();

            // Rewind the stack.
            vm->frame_count--;
            vm->stack_top = frame.slots;
            Push(result);

            frame = vm->frames[vm->frame_count - 1];
            Dispatch();
        }

      Case(OP_EXIT): {
            print_value(vm->x);
            putchar('\n');
            Pop(); // Top-level Wrapping Function
            vm->frame_count = 0;
            vm->x = Nil_Value;
            return INTERPRET_OK;
        }
    }
              
    assert(!"invalid instruction");
    return INTERPRET_RUNTIME_ERROR; // For warnings


#undef Binary_OP
#undef Runtime_Error
#undef Save_Frame
#undef Peek
#undef Pop
#undef Push
#undef Read_Offset
#undef Read_String
#undef Read_Constant
#undef Read_Byte
#undef Dispatch
#undef Case
#undef Start
#undef Log_Execution
}

InterpretResult interpret(VM *vm, const char *source, const char *path) {
    ObjFunction *function = compile(vm, source, path);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    // The compiler already reserve this slot for the function.
    push(vm, Obj_Value(function));

    // Top-level code is wrapped in a function for convenience,
    // to run the code, we simply call the wapping function.
    push_frame(vm, function, 0);

    vm->path = path;
    return run_vm(vm);
}
