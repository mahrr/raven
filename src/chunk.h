#ifndef raven_chunk_h
#define raven_chunk_h

#include "common.h"

typedef enum {
    OP_RETURN,
} Opcode;

// Line encoding
typedef struct {
    int line;
    int offset;
} Line;

typedef struct {
    // Dynamic array of the opcodes.
    int count;
    int capacity;
    uint8_t *opcodes;

    // Dynamic array of the lines corresponding to opcodes.
    // This array is not in sync with the opcodes array, the
    // lines are encoded in some sort of Run-length format.
    int lines_count;
    int lines_capacity;
    Line *lines;
} Chunk;

// Initialize the chunk state.
void init_chunk(Chunk *chunk);

// Free the chunk memory.
void free_chunk(Chunk *chunk);

// Add a byte to the chuck, and register it with the provided line.
void write_byte(Chunk *chunk, uint8_t byte, int line);

// Decode a line corresponing to a given instruction offset
int decode_line(Chunk *chunk, int offset);

#endif
