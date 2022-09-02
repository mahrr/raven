#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <errno.h>

#include "common.h"
#include "compiler.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
#include "vm.h"

#ifdef DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif

/// Runtime Errors

static const char* type_repr(Value value) {
    if (Is_Nil(value))       return "nil";
    if (Is_Bool(value))      return "boolean";
    if (Is_Num(value))       return "number";
    if (Is_String(value))    return "string";
    if (Is_Pair(value))      return "pair";
    if (Is_Array(value))     return "array";
    if (Is_Map(value))       return "map";
    if (Is_Function(value))  return "function";
    if (Is_Closure(value))   return "function";
    if (Is_CFunction(value)) return "native-function";

    assert(!"unreachable: invalid value tag");
    return NULL;
}

static inline void reset_stack(VM *vm) {
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
}

static void dump_stack_trace(VM *vm, FILE *out) {
    fprintf(out, "stack traceback:\n");

    // TODO: an option to control the stack trace dumping order.
    for (int i = vm->frame_count - 1; i >= 0; i--) {
        CallFrame *frame = &vm->frames[i];
        RavFunction *function = frame->closure->function;

        size_t offset = frame->ip - function->chunk.opcodes - 1;
        int line = chunk_decode_line(&function->chunk, offset);

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
    int line = chunk_decode_line(&function->chunk, offset);
    fprintf(stderr, "[%s | line: %d] ", vm->path, line);

    vfprintf(stderr, format, arguments);
    putc('\n', stderr);

    dump_stack_trace(vm, stderr);

    va_end(arguments);
    reset_stack(vm);
}

/// Stack Operations

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
        runtime_error(vm, "expect %d arguments, but got %d", function->arity, count);
        return false;
    }

    return push_frame(vm, closure, count);
}

static bool call_cfunction(VM *vm, RavCFunction *cfunction, int count) {
    int arity_min = cfunction->arity_min;
    int arity_max = cfunction->arity_max;

    if (count < arity_min || count > arity_max) {
        if (arity_min == arity_max) { // non-variadic
            runtime_error(vm, "expect %d arguments, but got %d", arity_min, count);
        } else if (arity_max != PARAMS_LIMIT) { // non-variadic
            runtime_error(vm, "expect %d to %d arguments, but got %d", arity_min, arity_max, count);
        } else { // variadic
            runtime_error(vm, "expect at least %d arguments, but got %d", arity_min, count);
        }
        return false;
    }

    Value result = Nil_Value;

    Value *stack_before = vm->stack_top;
    bool succeed = cfunction->func(vm, vm->stack_top - count, count, &result);
    Value *stack_after = vm->stack_top;

    // an error has occured in the function call
    if (!succeed) {
        return false;
    }

    // native functions must not manipulate the stack at a level before their
    // passed arguments
    if (stack_after < (stack_before - count)) {
        runtime_error(vm, "corrupted stack");
        return false;
    }

    // rewind the stack to the point before the cfunction and its argument
    vm->stack_top = stack_before - (count + 1);

    // push the returned value from the function
    push(vm, result);
    return true;
}

static bool call_value(VM *vm, Value value, int count) {
    if (Is_Closure(value)) {
        return call_closure(vm, As_Closure(value), count);
    }
    if (Is_CFunction(value)) {
        return call_cfunction(vm, As_CFunction(value), count);
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

    RavUpvalue *upvalue = object_upvalue(&vm->allocator, location);
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

/// VM Dispatch Loop

static InterpretResult run_vm(register VM *vm) {
    CallFrame frame = vm->frames[vm->frame_count - 1];
    uint8_t instruction;

#ifdef DEBUG_TRACE_EXECUTION
#define Log_Execution()                                                  \
    do {                                                                 \
        printf("          ");                                            \
        for (Value *value = vm->stack; value < vm->stack_top; value++) { \
            printf("[ ");                                                \
            value_print(*value);                                         \
            printf(" ]");                                                \
        }                                                                \
        printf(" X = ");                                                 \
        value_print(vm->x);                                              \
        putchar('\n');                                                   \
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

        Push(Bool_Value(value_equal(x, y)));
        Dispatch();
    }

    Case(OP_NEQ): {
        Value y = Pop();
        Value x = Pop();

        Push(Bool_Value(!value_equal(x, y)));
        Dispatch();
    }

    Case(OP_LT):  Binary_OP(Bool_Value, <);  Dispatch();
    Case(OP_LTQ): Binary_OP(Bool_Value, <=); Dispatch();
    Case(OP_GT):  Binary_OP(Bool_Value, >);  Dispatch();
    Case(OP_GTQ): Binary_OP(Bool_Value, >=); Dispatch();

    Case(OP_NOT): Push(Bool_Value(is_falsy(Pop()))); Dispatch();

    Case(OP_CONCATENATE): {
        // peek is used instead of pop, because of the possible GC round in `string_buf_push`
        // call could reclaim any of `left` or `right` memory, if they were objects
        Value left = Peek(1);
        Value right = Peek(0);

        StringBuffer buffer = string_buf_new(&vm->allocator);
        string_buf_push(&buffer, left);
        string_buf_push(&buffer, right);
        Value result = Obj_Value(string_buf_into(&buffer));

        (void) Pop(); // right
        (void) Pop(); // left
        Push(result);

        Dispatch();
    }

    Case(OP_CONS): {
        // peek is used instead of pop, because of the possible GC round in `object_pair`
        // call could reclaim any of `tail` or `head` memory, if they were objects
        Value tail = Peek(0);
        Value head = Peek(1);
        Value pair = Obj_Value(object_pair(&vm->allocator, head, tail));

        (void) Pop(); // head
        (void) Pop(); // tail
        Push(pair);

        Dispatch();
    }

    Case(OP_ARRAY_8): {
        size_t count = (size_t)Read_Byte();
        RavArray *array = object_array(&vm->allocator, vm->stack_top - count, count);
        vm->stack_top -= count;
        Push(Obj_Value(array));

        Dispatch();
    }

    Case(OP_ARRAY_16): {
        size_t count = (size_t)Read_Short();
        RavArray *array = object_array(&vm->allocator, vm->stack_top - count, count);
        vm->stack_top -= count;
        Push(Obj_Value(array));

        Dispatch();
    }

    Case(OP_MAP_8): {
        size_t count = (size_t)Read_Byte() * 2;
        Value *offset = vm->stack_top - count;
        RavMap *map = object_map(&vm->allocator);

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
        RavMap *map = object_map(&vm->allocator);

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
        RavString *name = Read_String();
        table_set(&vm->globals, name, Peek(0));
        Pop();
        Dispatch();
    }

    Case(OP_SET_GLOBAL): {
        RavString *name = Read_String();
        if (table_set(&vm->globals, name, Peek(0))) {
            table_remove(&vm->globals, name);
            Runtime_Error("unbound variable '%s'", name->chars);
            return INTERPRET_RUNTIME_ERROR;
        }
        Dispatch();
    }

    Case(OP_GET_GLOBAL): {
        RavString *name = Read_String();

        Value value;
        if (table_get(&vm->globals, name, &value) == false) {
            Runtime_Error("unbound variable '%s'", name->chars);
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
        RavClosure *closure = object_closure(&vm->allocator, function);
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
        Pop(); // Top-level Wrapping Function
        if (vm->reset_on_exit)
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

/// Native Functions

static void register_natives(VM*);

static bool native_import(VM *vm, Value *arguments, size_t count, Value *result) {
    MAYBE_UNUSED(count);

    Value argument = arguments[0];
    if (Is_String(argument) == false) {
        runtime_error(vm, "`import` expected string, got %s", type_repr(argument));
        return false;
    }

    // read the file
    const char *path = As_String(argument)->chars;
    char *source = NULL;
    {
        FILE *file = fopen(path, "rb");
        if (file == NULL) {
            runtime_error(vm, "`import` error reading '%s' (%s)", path, strerror(errno));
            return false;
        }

        fseek(file, 0L, SEEK_END);
        size_t size = ftell(file);
        rewind(file);

        source = (char *)malloc(size + 1);
        if (source == NULL) {
            runtime_error(vm, "`import` error reading '%s' (out of memory)", path);
            fclose(file);
            return false;
        }

        size_t bytes_read = fread(source, sizeof (char), size, file);
        if (bytes_read < size || ferror(file)) {
            runtime_error(vm, "`import` error reading '%s' (%s)", path, strerror(errno));
            fclose(file);
            free(source);
            return false;
        }
        fclose(file);
        source[size] = '\0';
    }

    // execute the source
    Value exported = Nil_Value;
    {
        VM sandbox = {0};
        sandbox.reset_on_exit = false;
        sandbox.allocator = vm->allocator;
        sandbox.path = path;
        sandbox.x = Nil_Value;

        reset_stack(&sandbox);
        table_init(&sandbox.globals);
        register_natives(&sandbox);

        // since the current context's object are not reachable to the sandbox they need
        // to be hided, so they are not freed in the `interpret` call by the GC, this is
        // achieved by marking the beginning of the current objects list to be the end of
        // the sandbox object list
        sandbox.allocator.objects_end = sandbox.allocator.objects;

        RavFunction *function = compile(&sandbox, source, path);
        if (function == NULL) {
            free(source);
            return false;
        }

        RavClosure *closure = object_closure(&sandbox.allocator, function);
        push(&sandbox, Obj_Value(closure));
        push_frame(&sandbox, closure, 0);

        // TODO: errors should dump the current context stack
        InterpretResult result = run_vm(&sandbox);
        if (result != INTERPRET_OK) {
            free(source);
            return false;
        }

        // reset allocator state to the current context
        sandbox.allocator.objects_end = NULL;
        vm->allocator = sandbox.allocator;

        // obtian the exported value from the X register
        exported = sandbox.x;

        // clear the sandbox resources
        table_free(&sandbox.globals);
    }

    *result = exported;

    free(source);
    return true;
}

static bool native_assert(VM *vm, Value *arguments, size_t count, Value *result) {
    Value expression = arguments[0];
    Value message = Nil_Value;

    // message is optional
    if (count == 2) {
        message = arguments[1];

        if (Is_String(message) == false) {
            runtime_error(vm, "`assert` expected string as second argument, got %s", type_repr(message));
            return false;
        }
    }

    if (is_falsy(expression)) {
        if (count == 2) {
            runtime_error(vm, "assertion failed: %s", As_String(message)->chars);
        } else {
            runtime_error(vm, "assertion failed");
        }
        return false;
    }

    *result = Nil_Value;
    return true;
}

static bool native_print(VM *vm, Value *arguments, size_t count, Value *result) {
    MAYBE_UNUSED(vm);

    for (size_t i = 0; i < count; ++i) {
        value_print(arguments[i]);
        putchar(' ');
    }

    *result = Nil_Value;
    return true;
}

static bool native_println(VM *vm, Value *arguments, size_t count, Value *result) {
    MAYBE_UNUSED(vm);

    for (size_t i = 0; i < count; ++i) {
        value_print(arguments[i]);
        putchar(' ');
    }
    putchar('\n');

    *result = Nil_Value;
    return true;
}

static bool native_len(VM *vm, Value *arguments, size_t count, Value *result) {
    MAYBE_UNUSED(count);

    Value argument = arguments[0];
    if (Is_String(argument)) {
        *result = Num_Value(As_String(argument)->length);
        return true;
    }
    if (Is_Array(argument)) {
        *result = Num_Value(As_Array(argument)->count);
        return true;
    }
    if (Is_Map(argument)) {
        *result = Num_Value(As_Map(argument)->table.count);
        return true;
    }

    runtime_error(vm, "`len` expected collection, got %s", type_repr(argument));
    return false;
}

// Array Native Functions

static bool native_push(VM *vm, Value *arguments, size_t count, Value *result) {
    Value argument = arguments[0];
    if (Is_Array(argument) == false) {
        runtime_error(vm, "`push` expected array as firt argument, got %s", type_repr(argument));
        return false;
    }

    RavArray *array = As_Array(argument);
    for (size_t i = 1; i < count; ++i) {
        // Grow array memory, if needed
        if (array->count == array->capacity) {
            size_t old_cap = array->capacity;
            size_t new_cap = Grow_Capacity(old_cap);
            array->capacity = new_cap;
            array->values = Grow_Array(&vm->allocator, array->values, Value, old_cap, new_cap);
        }
        array->values[array->count++] = arguments[i];
    }

    *result = argument;
    return true;
}

static bool native_pop(VM *vm, Value *arguments, size_t count, Value *result) {
    MAYBE_UNUSED(count);

    Value argument = arguments[0];
    if (Is_Array(argument) == false) {
        runtime_error(vm, "`pop` expected array, got %s", type_repr(argument));
        return false;
    }

    RavArray *array = As_Array(argument);
    if (array->count == 0) {
        runtime_error(vm, "`pop` on an empty array");
        return false;
    }

    *result = array->values[--array->count];
    return true;
}

// Map Native Functions

static bool native_insert(VM *vm, Value *arguments, size_t count, Value *result) {
    MAYBE_UNUSED(count);

    Value argument1 = arguments[0];
    if (Is_Map(argument1) == false) {
        runtime_error(vm, "`insert` expected map as first argument, got %s", type_repr(argument1));
        return false;
    }
    RavMap *map = As_Map(argument1);

    Value argument2 = arguments[1];
    if (Is_String(argument2) == false) {
        runtime_error(vm, "`insert` expected string as second argument, got %s", type_repr(argument2));
        return false;
    }
    RavString *key = As_String(argument2);

    Value value = arguments[2];
    table_set(&map->table, key, value);
    *result = value;
    return true;
}

static bool native_remove(VM *vm, Value *arguments, size_t count, Value *result) {
    MAYBE_UNUSED(count);

    Value argument1 = arguments[0];
    if (Is_Map(argument1) == false) {
        runtime_error(vm, "`remove` expected map as firt argument, got %s", type_repr(argument1));
        return false;
    }
    RavMap *map = As_Map(argument1);

    Value argument2 = arguments[1];
    if (Is_String(argument2) == false) {
        runtime_error(vm, "`remove` expected string as second argument, got %s", type_repr(argument2));
        return false;
    }
    RavString *key = As_String(argument2);

    *result = table_remove(&map->table, key);
    return true;
}

static void register_natives(VM* vm) {
    vm->allocator.gc_off = true;

#define Register(name, arity_min, arity_max)                                                        \
    do {                                                                                            \
        RavCFunction *func = object_cfunction(&vm->allocator, native_##name, arity_min, arity_max); \
        RavString *name_string = object_string(&vm->allocator, #name, strlen(#name));               \
        table_set(&vm->globals, name_string, Obj_Value(func));                                      \
    } while (false)

    Register(import,  1, 1);
    Register(assert,  1, 2);
    Register(print,   0, PARAMS_LIMIT); // variadic
    Register(println, 0, PARAMS_LIMIT); // variadic
    Register(len,     1, 1);
    Register(push,    2, PARAMS_LIMIT); // variadic
    Register(pop,     1, 1);
    Register(insert,  3, 3);
    Register(remove,  2, 2);

#undef Register
}

/// VM API

void init_vm(VM *vm) {
    vm->open_upvalues = NULL;
    vm->reset_on_exit = true;

    allocator_init(&vm->allocator);
    table_init(&vm->globals);
    reset_stack(vm);
    register_natives(vm);
}

void free_vm(VM *vm) {
    table_free(&vm->globals);
    allocator_free(&vm->allocator);
    *vm = (VM){0};
}

InterpretResult interpret(VM *vm, const char *source, const char *path) {
    // Disable the GC while compiling.
    vm->allocator.gc_off = true;
    vm->x = Nil_Value;

    RavFunction *function = compile(vm, source, path);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    RavClosure *closure = object_closure(&vm->allocator, function);
    push(vm, Obj_Value(closure));

    // Top-level code is wrapped in a function for convenience,
    // to run the code, we simply call the wrapping function.
    push_frame(vm, closure, 0);

    vm->path = path;
    vm->allocator.gc_off = false;
    return run_vm(vm);
}
