/*
 *
 * (lerror.c | 28 Nov 18 | Ahmad Maher)
 *
 * See lerror.h for full description.
*/

/*
 * ##Note##
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

    fprintf(stderr, "[%s @ %ld] syntax error: ",
            tok->file, tok->line);
    
    switch (tok->type) {
    case UNRECOG:
        fprintf(stderr, "unrecognize syntax '%s'\n", tok->lexeme);
        break;
    case UNTERMIN_COMM:
        fprintf(stderr, "unterminated comment at line %ld\n",
                tok->line);
        break;
    case UNTERMIN_STR:
        fprintf(stderr, "unterminated string at line %ld\n",
                tok->line);
        break;
    case UNTERMIN_RSTR:
        fprintf(stderr, "unterminated long string at line %ld\n",
                tok->line);
    case INVALID_ESCP:
        fprintf(stderr, "\"%s\" invalid escape sequence\n",
                tok->lexeme);
        break;
    case OCT_OUTOFR_ESCP:
        fprintf(stderr, "\"%s\" out of range escape sequence\n",
                tok->lexeme);
        break;
    case OCT_MISS_ESCP:
    case HEX_MISS_ESCP:
        fprintf(stderr, "\"%s\" missing digits for the escape sequence\n",
                tok->lexeme);
        break;
    case OCT_INVL_ESCP:
    case HEX_INVL_ESCP:
        fprintf(stderr, "\"%s\" invalid format for the escape sequence\n",
                tok->lexeme);
        break;  
    case ZERO_START:
        fprintf(stderr, "decimal numbers can't start with zero.\n");
        break;
    }
}
