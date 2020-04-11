#include "chunk.h"
#include "debug.h"

int main(void) {
    Chunk chunk;

    init_chunk(&chunk);
    write_byte(&chunk, OP_ADD, 1);
    write_byte(&chunk, OP_SUB, 1);
    write_byte(&chunk, OP_MUL, 1);
    write_byte(&chunk, OP_DIV, 2);
    write_byte(&chunk, OP_MOD, 2);
    write_byte(&chunk, OP_NIL, 3);
    write_byte(&chunk, OP_TRUE, 3);
    write_byte(&chunk, OP_FALSE, 3);
    write_byte(&chunk, OP_RETURN, 4);

    disassemble_chunk(&chunk, "top-level");
    free_chunk(&chunk);

    return 0;
}
