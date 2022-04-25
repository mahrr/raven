#ifndef raven_common_h
#define raven_common_h

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Debugging flags
#ifdef DEBUG
//# define DEBUG_TRACE_PARSING   // Dump the parsing functions call stack
# define DEBUG_TRACE_EXECUTION // Dump the vm state on every instruction
//# define DEBUG_TRACE_MEMORY    // Dump the memory info on GC/Alloc/Free
# define DEBUG_STRESS_GC       // Trigger the GC on every allocation
# define DEBUG_DUMP_CODE       // Dump the functions compiled chunk
#endif

// If using GCC or Clang, use computed-goto for the virtual machine
// dispatch loop, otherwise fallback to the standatd switch statement.
#if (defined(__GNUC__) || defined(__CLANG__))
# define THREADED_CODE
#endif

// If it's x64_86 architecture use NaN tagging for the value
// representation. This method was first defined in the paper
// 'Representing Type Information in Dynamically Typed Languages'.
#if defined(__amd64__) || defined(__x86_64__) || defined(_M_AMD64)
# define NAN_TAGGING
#endif

// System Configuration

// The limit of nested frames.
#define FRAMES_LIMIT 128

// Maximum number of values on the stack.
#define STACK_SIZE (256 * FRAMES_LIMIT)

// The limit of number of locals per function.
#define LOCALS_LIMIT UINT8_MAX + 1

// The limit of number of variables a closure can capture.
#define UPVALUES_LIMIT UINT8_MAX + 1

// The limit of number of globals per script.
#define GLOBALS_LIMIT UINT8_MAX + 1

// The limit of number of constants per function.
#define CONST_LIMIT UINT8_MAX + 1

// The limie of number of parameters a function can have.
#define PARAMS_LIMIT UINT8_MAX + 1

// The limit of number of elements in an array literal.
#define ARRAY_LIMIT UINT16_MAX + 1

// The limit of number of elements in a map literal.
#define MAP_LIMIT UINT16_MAX + 1

// The limit of cond cases.
#define COND_LIMIT 256

#endif
