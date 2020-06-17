// Virtual Machine Instructions

// Instruction Opcode     // Immediate Operand

// Stack Operations
Opcode(OP_PUSH_TRUE)
Opcode(OP_PUSH_FALSE)
Opcode(OP_PUSH_NIL)
Opcode(OP_PUSH_CONST)     // 1-byte constant index

// TODO: optimize the STORE LOAD STORE pattern.
Opcode(OP_PUSH_X)
Opcode(OP_STORE_X)

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

// Variables
Opcode(OP_DEF_GLOBAL)     // 1-byte name index
Opcode(OP_SET_GLOBAL)     // 1-byte name index
Opcode(OP_GET_GLOBAL)     // 1-byte name index
Opcode(OP_SET_LOCAL)      // 1-byte slot index
Opcode(OP_GET_LOCAL)      // 1-byte slot index

// Branching
Opcode(OP_CALL)           // 1-byte arguments count
Opcode(OP_JMP)            // 2-bytes offset
Opcode(OP_JMP_BACK)       // 2-bytes offset
Opcode(OP_JMP_FALSE)      // 2-bytes offset
Opcode(OP_JMP_POP_FALSE)  // 2-bytes offset

Opcode(OP_RETURN)
Opcode(OP_EXIT)
