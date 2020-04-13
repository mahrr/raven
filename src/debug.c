#include <stdio.h>

#include "chunk.h"
#include "debug.h"

static int basic_instruction(const char *tag, int offset) {
    printf("%s\n", tag);
    return offset + 1;
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
    case OP_TRUE:    return basic_instruction("TRUE", offset);
    case OP_FALSE:   return basic_instruction("FALSE", offset);
    case OP_NIL:     return basic_instruction("NIL", offset);
    case OP_ADD:     return basic_instruction("ADD", offset);
    case OP_SUB:     return basic_instruction("SUB", offset);
    case OP_MUL:     return basic_instruction("MUL", offset);
    case OP_DIV:     return basic_instruction("DIV", offset);
    case OP_MOD:     return basic_instruction("MOD", offset);
    case OP_RETURN:  return basic_instruction("RETURN", offset);
    }

    assert(0);
}

void disassemble_chunk(Chunk *chunk, const char *name) {
    printf("## %s ##\n", name);

    for (int offset = 0; offset < chunk->count; ) {
        offset = disassemble_instruction(chunk, offset);
    }
}
