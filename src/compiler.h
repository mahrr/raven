#ifndef raven_compiler_h
#define raven_compiler_h

#include "common.h"
#include "vm.h"

// Compile a given source code to bytecode instructions,
// Return a function object containing the output chunk,
// or NULL on compilation error.
RavFunction *compile(VM *vm, const char *source, const char *file);

#endif
