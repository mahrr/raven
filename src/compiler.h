#ifndef raven_compiler_h
#define raven_compiler_h

#include "common.h"
#include "vm.h"

// Compile a given source code to bytecode instructions,
// and dump these instructions to the given vm chunk.
// Return true on success compilation, otherwise false.
bool compile(VM *vm, const char *source, const char *file);

#endif
