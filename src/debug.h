#ifndef raven_debug_h
#define raven_debug_h

#include "chunk.h"

// Dump out a chunk instruction at given offset.
// Return the offset of the next instruction.
int disassemble_instruction(Chunk *chunk, int offset);

// Dump out a whole chunk instructions with a given tag name.
void disassemble_chunk(Chunk *chunk, const char *path, const char *name);

#endif
