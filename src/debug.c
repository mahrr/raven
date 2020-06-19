#include <stdio.h>

#include "chunk.h"
#include "debug.h"

static int basic_instruction(const char *tag, int offset) {
    printf("%s\n", tag);
    return offset + 1;
}

static int const_instruction(const char *tag, Chunk *chunk, int offset) {
    uint8_t constant_index = chunk->opcodes[offset + 1];
    printf("%-16s %4d '", tag, constant_index);
    print_value(chunk->constants[constant_index]);
    printf("'\n");
    return offset + 2;
}

static int byte_instruction(const char *tag, Chunk *chunk, int offset) {
    uint8_t count = chunk->opcodes[offset + 1];
    printf("%-16s %4d\n", tag, count);
    return offset + 2;
}

static int jump_instruction(const char *tag, Chunk *chunk, int sign,
                            int offset) {
    uint16_t jump = (uint16_t)(chunk->opcodes[offset + 1] << 8 |
                               chunk->opcodes[offset + 2]);

    printf("%-16s %4d -> %d\n", tag, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int disassemble_instruction(Chunk *chunk, int offset) {
    printf("%04d", offset);

    int line = decode_line(chunk, offset);
    int instruction = chunk->opcodes[offset];
        
    if (offset > 0 && line == decode_line(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", line);
    }
    
    switch (instruction) {
    case OP_PUSH_TRUE:
        return basic_instruction("PUSH_TRUE", offset);
        
    case OP_PUSH_FALSE:
        return basic_instruction("PUSH_FALSE", offset);
        
    case OP_PUSH_NIL:
        return basic_instruction("PUSH_NIL", offset);
        
    case OP_PUSH_CONST:
        return const_instruction("PUSH_CONST", chunk, offset);

    case OP_PUSH_X:
        return basic_instruction("PUSH_X", offset);
        
    case OP_SAVE_X:
        return basic_instruction("SAVE_X", offset);

    case OP_POP:
        return basic_instruction("POP", offset);

    case OP_POPN:
        return byte_instruction("POPN", chunk, offset);
        
    case OP_ADD:
        return basic_instruction("ADD", offset);
        
    case OP_SUB:
        return basic_instruction("SUB", offset);
        
    case OP_MUL:
        return basic_instruction("MUL", offset);
        
    case OP_DIV:
        return basic_instruction("DIV", offset);
        
    case OP_MOD:
        return basic_instruction("MOD", offset);

    case OP_NEG:
        return basic_instruction("NEG", offset);

    case OP_EQ:
        return basic_instruction("EQ", offset);

    case OP_NEQ:
        return basic_instruction("NEQ", offset);

    case OP_LT:
        return basic_instruction("LT", offset);

    case OP_LTQ:
        return basic_instruction("LTQ", offset);

    case OP_GT:
        return basic_instruction("GT", offset);

    case OP_GTQ:
        return basic_instruction("GTQ", offset);

    case OP_NOT:
        return basic_instruction("NOT", offset);

    case OP_DEF_GLOBAL:
        return byte_instruction("DEF_GLOBAL", chunk, offset);

    case OP_SET_GLOBAL:
        return byte_instruction("SET_GLOBAL", chunk, offset);

    case OP_GET_GLOBAL:
        return byte_instruction("GET_GLOBAL", chunk, offset);

    case OP_SET_LOCAL:
        return byte_instruction("SET_LOCAL", chunk, offset);

    case OP_GET_LOCAL:
        return byte_instruction("GET_LOCAL", chunk, offset);

    case OP_CALL:
        return byte_instruction("CALL", chunk, offset);

    case OP_JMP:
        return jump_instruction("JMP", chunk, 1, offset);

    case OP_JMP_BACK:
        return jump_instruction("JMP_BACK", chunk, -1, offset);

    case OP_JMP_FALSE:
        return jump_instruction("JMP_FALSE", chunk, 1, offset);

    case OP_JMP_POP_FALSE:
        return jump_instruction("JMP_POP_FALSE", chunk, 1, offset);
        
    case OP_RETURN:
        return basic_instruction("RETURN", offset);

    case OP_EXIT:
        return basic_instruction("EXIT", offset);
    }

    assert(!"invalid instruction type");
    return 0; // For Warnings
}

void disassemble_chunk(Chunk *chunk, const char *name) {
    printf("\n== %s ==\n", name);

    for (int offset = 0; offset < chunk->count; ) {
        offset = disassemble_instruction(chunk, offset);
    }
}
