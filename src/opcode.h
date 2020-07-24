// Virtual Machine Instructions

// Instruction Opcode     // Immediate Operand

// Stack Operations
Opcode(OP_PUSH_TRUE)
Opcode(OP_PUSH_FALSE)
Opcode(OP_PUSH_NIL)
Opcode(OP_PUSH_CONST)     // 1-byte constant index

Opcode(OP_PUSH_X)
Opcode(OP_SAVE_X)

Opcode(OP_POP)
Opcode(OP_POPN)           // 1-byte count
    
// Arithmetics
Opcode(OP_ADD)
Opcode(OP_SUB)
Opcode(OP_MUL)
Opcode(OP_DIV)
Opcode(OP_MOD)
Opcode(OP_NEG)

// Comparison
Opcode(OP_EQ)
Opcode(OP_NEQ)
Opcode(OP_LT)
Opcode(OP_LTQ)
Opcode(OP_GT)
Opcode(OP_GTQ)

Opcode(OP_NOT)

// Collections
Opcode(OP_CONS)
Opcode(OP_ARRAY_8)        // 1-byte number of elements
Opcode(OP_ARRAY_16)       // 2-byte number of elements
Opcode(OP_INDEX_SET)
Opcode(OP_INDEX_GET)

// Variables
Opcode(OP_DEF_GLOBAL)     // 1-byte global buffer index
Opcode(OP_SET_GLOBAL)     // 1-byte global buffer index
Opcode(OP_GET_GLOBAL)     // 1-byte global buffer index
Opcode(OP_SET_LOCAL)      // 1-byte stack slot index
Opcode(OP_GET_LOCAL)      // 1-byte stack slot index
Opcode(OP_SET_UPVALUE)    // 1-byte upvalue list index
Opcode(OP_GET_UPVALUE)    // 1-byte upvalue list index

// Branching
Opcode(OP_CALL)           // 1-byte arguments count
Opcode(OP_JMP)            // 2-bytes offset
Opcode(OP_JMP_BACK)       // 2-bytes offset
Opcode(OP_JMP_FALSE)      // 2-bytes offset
Opcode(OP_JMP_POP_FALSE)  // 2-bytes offset

// Closure
Opcode(OP_CLOSURE)        // 1-byte function index
Opcode(OP_CLOSE_UPVALUE)

Opcode(OP_ASSERT)
Opcode(OP_RETURN)
Opcode(OP_EXIT)
