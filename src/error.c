/*
 *
 * (lerror.c | 28 Nov 18 | Ahmad Maher)
 *
 * See error.h for full description.
*/

/*
 * ::NOTE::
 *
 * I'm keeping the implementation simple for now.
 * It could be improved by printing the source line
 * where the error occur, and point to the start of
 * error. but i will defer that for later.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"

extern void fatal_error(int status, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(status);
}

extern void lex_error(token *tok) {

    fprintf(stderr, "[%s@%ld] syntax error: ",
            tok->file, tok->line);
    
    switch (tok->type) {
    case TK_UNRECOG:
        fprintf(stderr, "unrecognize syntax '%s'\n", tok->lexeme);
        break;
    case TK_UNTERMIN_STR:
        fprintf(stderr, "unterminated string at line (%ld)\n",
                tok->line);
        break;
    case TK_INVALID_ESCP:
        fprintf(stderr, "invalid escape sequence in %s\n",
                tok->lexeme);
        break;
    case TK_OCT_OUTOFR_ESCP:
        fprintf(stderr, "out of range escape sequence in %s\n",
                tok->lexeme);
        break;
    case TK_OCT_MISS_ESCP:
    case TK_HEX_MISS_ESCP:
        fprintf(stderr,
                "missing digits for the escape sequence in %s\n",
                tok->lexeme);
        break;
    case TK_OCT_INVL_ESCP:
    case TK_HEX_INVL_ESCP:
        fprintf(stderr,
                "invalid format for the escape sequence in %s\n",
                tok->lexeme);
        break;
    case TK_INVALID_SCIEN:
        fprintf(stderr, "malformed scientific notation in '%s'\n",
                tok->lexeme);
    default:
        return; /* just for warnings */
    }
}
