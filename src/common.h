#ifndef raven_common_h
#define raven_common_h

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Debugging flags
#ifdef DEBUG
# define DEBUG_TRACE_PARSING
# define DEBUG_TRACE_EXECUTION
# define DEBUG_TRACE_GC
# define DEBUG_DUMP_CODE
#endif

// If using GCC or Clang, use computed-goto for the virtual machine
// dispatch loop, otherwise fallback to the standatd switch statement.
#if defined(__GNUC__) || defined(__CLANG__)
# define THREADED_CODE
#endif

#endif
