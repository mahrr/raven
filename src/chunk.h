#ifndef raven_chunk_h
#define raven_chunk_h

#include "common.h"
#include "value.h"

typedef enum {      // Immediate Arguments
    // Loading
    OP_LOAD_TRUE,
    OP_LOAD_FALSE,
    OP_LOAD_NIL,
    OP_LOAD_CONST,  // constant index
    
    // Arithmetics
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEG,

    // Logic
    OP_NOT,
    
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

    ValueArray constants;
} Chunk;

// Initialize the chunk state.
void init_chunk(Chunk *chunk);

// Free the chunk memory.
void free_chunk(Chunk *chunk);

// Add a byte to the chuck, and register it with the provided line.
void write_byte(Chunk *chunk, uint8_t byte, int line);

// Add a constant to the constants table, and return its index.
int write_constant(Chunk *chunk, Value value);

// Decode a line corresponing to a given instruction offset
int decode_line(Chunk *chunk, int offset);

#endif
