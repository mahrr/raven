#include <stdio.h>
#include <stdarg.h>
#include <math.h>

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
    vm->x = Nil_Value;
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
}

void init_vm(VM *vm) {
    vm->frame_count = 0;
    vm->open_upvalues = NULL;

    init_allocator(&vm->allocator);
    init_table(&vm->globals);
    reset_stack(vm);
}

void free_vm(VM *vm) {
    free_table(&vm->globals);
    free_allocator(&vm->allocator);

    init_vm(vm);
}

static void dump_stack_trace(VM *vm, FILE *out) {
    fprintf(out, "stack traceback:\n");

    // TODO: an option to control the stack trace dumping order.
    for (int i = vm->frame_count - 1; i >= 0; i--) {
        CallFrame *frame = &vm->frames[i];
        RavFunction *function = frame->closure->function;

        size_t offset = frame->ip - function->chunk.opcodes - 1;
        int line = decode_line(&function->chunk, offset);

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
    RavFunction *function = frame->closure->function;

    // -1 because ip is sitting on the next instruction to be executed.
    size_t offset = frame->ip - function->chunk.opcodes - 1;
    int line = decode_line(&function->chunk, offset);
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

static inline bool push_frame(VM *vm, RavClosure *closure, int count) {
    if (vm->frame_count == FRAMES_LIMIT) {
        runtime_error(vm, "call stack overflows");
        return false;
    }

    CallFrame *frame = &vm->frames[vm->frame_count++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.opcodes;
    frame->slots = vm->stack_top - count - 1;

    return true;
}

static bool call_closure(VM *vm, RavClosure *closure, int count) {
    RavFunction *function = closure->function;

    if (function->arity != count) {
        runtime_error(vm, "expect %d arguments, but got %d",
                      function->arity, count);
        return false;
    }

    return push_frame(vm, closure, count);
}

static inline bool call_value(VM *vm, Value value, int count) {
    if (Is_Closure(value)) {
        return call_closure(vm, As_Closure(value), count);
    }

    runtime_error(vm, "call to a non-callable");
    return false;
}

static RavUpvalue *capture_upvalue(VM *vm, Value *location) {
    RavUpvalue *previous = NULL;
    RavUpvalue *current = vm->open_upvalues;

    while (current != NULL && current->location > location) {
        previous = current;
        current = current->next;
    }

    if (current && current->location == location) return current;

    RavUpvalue *upvalue = new_upvalue(&vm->allocator, location);
    upvalue->next = current;

    if (previous == NULL) {
        vm->open_upvalues = upvalue;
    } else {
        previous->next = upvalue;
    }

    return upvalue;
}

static void close_upvalues(VM *vm, Value *slot) {
    while (vm->open_upvalues != NULL &&
           vm->open_upvalues->location >= slot) {
        RavUpvalue *upvalue = vm->open_upvalues;
        upvalue->captured = *upvalue->location;
        upvalue->location = &upvalue->captured;
        vm->open_upvalues = upvalue->next;
    }
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
    CallFrame frame = vm->frames[vm->frame_count - 1];
    uint8_t instruction;

#ifdef DEBUG_TRACE_EXECUTION
#define Log_Execution()                                                  \
    do {                                                                 \
        printf("          ");                                            \
        for (Value *value = vm->stack; value < vm->stack_top; value++) { \
            printf("[ ");                                                \
            print_value(*value);                                         \
            printf(" ]");                                                \
        }                                                                \
        printf("[ X: ");                                                 \
        print_value(vm->x);                                              \
        printf(" ]\n");                                                  \
                                                                         \
        RavFunction *function = frame.closure->function;                 \
        int offset = (int)(frame.ip - function->chunk.opcodes);          \
        disassemble_instruction(&function->chunk, offset);               \
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
#define Read_Short()                                                    \
    (frame.ip += 2, (uint16_t)(frame.ip[-2] << 8 | frame.ip[-1]))
#define Read_Constant()                                                 \
    (frame.closure->function->chunk.constants[Read_Byte()])
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
            Runtime_Error("operands must be numeric");       \
            return INTERPRET_RUNTIME_ERROR;                  \
        }                                                    \
                                                             \
        double y = As_Num(Pop());                            \
        double x = As_Num(Pop());                            \
                                                             \
        Push(value_type(x op y));                            \
    } while (false)

    Start() {
    Case(OP_PUSH_TRUE):  Push(Bool_Value(true));  Dispatch();
    Case(OP_PUSH_FALSE): Push(Bool_Value(false)); Dispatch();
    Case(OP_PUSH_NIL):   Push(Nil_Value);         Dispatch();
    Case(OP_PUSH_CONST): Push(Read_Constant());   Dispatch();

    Case(OP_PUSH_X): {
        Push(vm->x);
        vm->x = Nil_Value;
        Dispatch();
    }

    Case(OP_SAVE_X): {
        vm->x = Pop();
        Dispatch();
    }

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
    Case(OP_MOD): {
        if (!Is_Num(Peek(0)) && !Is_Num(Peek(1))) {
            Runtime_Error("operands must be numeric");
            return INTERPRET_RUNTIME_ERROR;
        }

        double y = As_Num(Pop());
        double x = As_Num(Pop());

        Push(Num_Value(fmod(x, y)));
        Dispatch();
    }
    Case(OP_NEG): {
        if (!Is_Num(Peek(0))) {
            Runtime_Error("negation operand must be numeric");
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

    Case(OP_CONS): {
        Value tail = Pop();
        Value head = Pop();

        Push(Obj_Value(new_pair(&vm->allocator, head, tail)));
        Dispatch();
    }

    Case(OP_ARRAY_8): {
        size_t count = (size_t)Read_Byte();
        RavArray *array = new_array(&vm->allocator, vm->stack_top - count, count);
        vm->stack_top -= count;
        Push(Obj_Value(array));

        Dispatch();
    }

    Case(OP_ARRAY_16): {
        size_t count = (size_t)Read_Short();
        RavArray *array = new_array(&vm->allocator, vm->stack_top - count, count);
        vm->stack_top -= count;
        Push(Obj_Value(array));

        Dispatch();
    }

    Case(OP_MAP_8): {
        size_t count = (size_t)Read_Byte() * 2;
        Value *offset = vm->stack_top - count;
        RavMap *map = new_map(&vm->allocator);

        for (size_t i = 0; i < count; i += 2) {
            Value key = offset[i];
            Value value = offset[i+1];

            table_set(&map->table, As_String(key), value);
        }

        vm->stack_top -= count;
        Push(Obj_Value(map));

        Dispatch();
    }

    Case(OP_MAP_16): {
        size_t count = (size_t)Read_Short() * 2;
        Value *offset = vm->stack_top - count;
        RavMap *map = new_map(&vm->allocator);

        for (size_t i = 0; i < count; i += 2) {
            Value key = offset[i];
            Value value = offset[i+1];

            table_set(&map->table, As_String(key), value);
        }

        vm->stack_top -= count;
        Push(Obj_Value(map));

        Dispatch();
    }

    Case(OP_INDEX_SET): {
        Value value = Pop();
        Value offset = Pop();
        Value collection = Pop();

        if (Is_Array(collection)) {
            RavArray *array = As_Array(collection);
            if (!Is_Num(offset)) {
                Runtime_Error("index an array with non-numeric type");
                return INTERPRET_RUNTIME_ERROR;
            }

            double index = As_Num(offset);
            if (index != floor(index)) {
                Runtime_Error("array index should not have fraction part (%f)", index);
                return INTERPRET_RUNTIME_ERROR;
            }

            if (index < 0.0f || index >= (double)array->count) {
                Runtime_Error("index out of bound (index: %lld, count: %llu)", (int64_t)index, array->count);
                return INTERPRET_RUNTIME_ERROR;
            }

            Push(array->values[(size_t)index] = value);
        } else if (Is_Map(collection)) {
            RavMap *map = As_Map(collection);
            if (!Is_String(offset)) {
                Runtime_Error("index a map with non-string type");
                return INTERPRET_RUNTIME_ERROR;
            }

            table_set(&map->table, As_String(offset), value);
            Push(value);
        } else {
            Runtime_Error("index a non-collection type");
            return INTERPRET_RUNTIME_ERROR;
        }

        Dispatch();
    }

    Case(OP_INDEX_GET): {
        Value offset = Pop();
        Value collection = Pop();

        if (Is_Array(collection)) {
            RavArray *array = As_Array(collection);
            if (!Is_Num(offset)) {
                Runtime_Error("index an array with non-numeric type");
                return INTERPRET_RUNTIME_ERROR;
            }

            double index = As_Num(offset);
            if (index != floor(index)) {
                Runtime_Error("array index should not have fraction part (%f)", index);
                return INTERPRET_RUNTIME_ERROR;
            }

            if (index < 0.0f || index >= (double)array->count) {
                Runtime_Error(
                    "index out of bound (index: %lld, count: %llu)",
                    (int64_t)index,
                    array->count
                );
                return INTERPRET_RUNTIME_ERROR;
            }

            Push(array->values[(size_t)index]);
        } else if (Is_Map(collection)) {
            RavMap *map = As_Map(collection);
            if (!Is_String(offset)) {
                Runtime_Error("index a map with non-string type");
                return INTERPRET_RUNTIME_ERROR;
            }

            Value value = Nil_Value;
            RavString* key = As_String(offset);
            table_get(&map->table, key, &value);

            Push(value);
        } else {
            Runtime_Error("index a non-collection type");
            return INTERPRET_RUNTIME_ERROR;
        }

        Dispatch();
    }

    Case(OP_DEF_GLOBAL): {
        vm->global_buffer[Read_Byte()] = Pop();
        Dispatch();
    }

    Case(OP_SET_GLOBAL): {
        uint8_t index = Read_Byte();

        if (Is_Void(vm->global_buffer[index])) {
            Runtime_Error("unbound variable '%s'", global_name_at(vm, index));
            return INTERPRET_RUNTIME_ERROR;
        }

        vm->global_buffer[index] = Peek(0);
        Dispatch();
    }

    Case(OP_GET_GLOBAL): {
        uint8_t index = Read_Byte();
        Value value = vm->global_buffer[index];

        if (Is_Void(value)) {
            Runtime_Error("unbound variable '%s'", global_name_at(vm, index));
            return INTERPRET_RUNTIME_ERROR;
        }

        Push(value);
        Dispatch();
    }

    Case(OP_SET_LOCAL): {
        frame.slots[Read_Byte()] = Peek(0);
        Dispatch();
    }

    Case(OP_GET_LOCAL): {
        Push(frame.slots[Read_Byte()]);
        Dispatch();
    }

    Case(OP_SET_UPVALUE): {
        *frame.closure->upvalues[Read_Byte()]->location = Peek(0);
        Dispatch();
    }

    Case(OP_GET_UPVALUE): {
        Push(*frame.closure->upvalues[Read_Byte()]->location);
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
        uint16_t offset = Read_Short();
        frame.ip += offset;
        Dispatch();
    }

    Case(OP_JMP_BACK): {
        uint16_t offset = Read_Short();
        frame.ip -= offset;
        Dispatch();
    }

    Case(OP_JMP_FALSE): {
        uint16_t offset = Read_Short();
        if (is_falsy(Peek(0))) frame.ip += offset;
        Dispatch();
    }

    Case(OP_JMP_POP_FALSE): {
        uint16_t offset = Read_Short();
        if (is_falsy(Pop())) frame.ip += offset;
        Dispatch();
    }

    Case(OP_CLOSURE): {
        RavFunction *function = As_Function(Read_Constant());
        RavClosure *closure = new_closure(&vm->allocator, function);
        Push(Obj_Value(closure));

        for (int i = 0; i < closure->upvalue_count; i++) {
            uint8_t is_local = Read_Byte();
            uint8_t index = Read_Byte();

            if (is_local) {
                closure->upvalues[i] = capture_upvalue(vm, frame.slots + index);
            } else {
                closure->upvalues[i] = frame.closure->upvalues[index];
            }
        }

        Dispatch();
    }

    Case(OP_CLOSE_UPVALUE): {
        close_upvalues(vm, vm->stack_top - 1);
        Pop();
        Dispatch();
    }

    Case(OP_ASSERT): {
        Value value = Pop();

        if (is_falsy(value)) {
            Runtime_Error("assertion failed");
            return INTERPRET_RUNTIME_ERROR;
        }

        vm->x = Nil_Value;
        Dispatch();
    }

    Case(OP_RETURN): {
        Value result = Pop();
        close_upvalues(vm, frame.slots);

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
        reset_stack(vm);
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
#undef Read_Short
#undef Read_String
#undef Read_Constant
#undef Read_Byte
#undef Dispatch
#undef Case
#undef Start
#undef Log_Execution
}

InterpretResult interpret(VM *vm, const char *source, const char *path) {
    // Disable the GC while compiling.
    vm->allocator.gc_off = true;

    RavFunction *function = compile(vm, source, path);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    // The compiler already reserve this slot for the function.
    // Push the function, so the GC doesn't free its memory
    // when allocating the closure object.
    RavClosure *closure = new_closure(&vm->allocator, function);
    push(vm, Obj_Value(closure));

    // Top-level code is wrapped in a function for convenience,
    // to run the code, we simply call the wrapping function.
    push_frame(vm, closure, 0);

    vm->path = path;
    vm->allocator.gc_off = false;
    return run_vm(vm);
}
