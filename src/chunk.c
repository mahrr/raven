#include "chunk.h"
#include "common.h"
#include "mem.h"
#include "value.h"

void init_chunk(Chunk *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->opcodes = NULL;

    chunk->lines_count = 0;
    chunk->lines_capacity = 0;
    chunk->lines = NULL;

    init_values(&chunk->constants);
}

void free_chunk(Chunk *chunk) {
    FREE_ARRAY(chunk->opcodes, uint8_t, chunk->capacity);
    FREE_ARRAY(chunk->lines, Line, chunk->lines_capacity);
}

static void write_line(Chunk *chunk, int line) {
    if (chunk->lines_count > 0 &&
        chunk->lines[chunk->lines_count - 1].line == line) return;

    if (chunk->lines_count == chunk->lines_capacity) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->lines = GROW_ARRAY(chunk->lines, Line,
                                  old_capacity, chunk->capacity);
    }

    Line *line_encoding = &chunk->lines[chunk->lines_count++];
    line_encoding->line = line;
    line_encoding->offset = chunk->count - 1;
}

void write_byte(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->count == chunk->capacity) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->opcodes = GROW_ARRAY(chunk->opcodes, uint8_t,
                                    old_capacity, chunk->capacity);
    }

    chunk->opcodes[chunk->count++] = byte;
    write_line(chunk, line);
}

int decode_line(Chunk *chunk, int offset) {
    int start = 0;
    int end = chunk->lines_count - 1;

    for (;;) {
        int mid = (start + end) / 2;
        Line *line = &chunk->lines[mid];
        
        if (offset < line->offset) {
            end = mid - 1;
        }  else if (mid == chunk->lines_count - 1 ||
                    offset < chunk->lines[mid + 1].offset) {
            return line->line;
        } else {
            start = mid + 1;
        }
    }
}