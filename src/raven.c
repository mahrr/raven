/*
 * (raven.c | 28 Nov 18 | Ahmad Maher)
 *
 * The main entry for the interpreter.
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "lexer.h"
#include "alloc.h"
#include "error.h"
#include "debug.h"

/* @@ just print tokens for now */

#define MAX_LINE 1024   /* the maximum size for repl line */

void repl() {
    char buf[MAX_LINE];
    Lexer l = lexer_new("", "", R_FIRS);

    for (;;) {
        fputs("#> ", stdout);
        fgets(buf, MAX_LINE, stdin);

        init_lexer(l, buf, "stdin");
        Token tok = cons_token(l);

        while (tok->type != TK_EOF) {
            if (tok->type == TK_ERR)
                lex_error(buf, tok);
            else
                print_token(tok);
            tok = cons_token(l);
        }
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        repl();
    } else {
        for (int i = 1; i < argc; i++) {
            /* load content file to src */
            char *src = scan_file(argv[i]);
            if (src == NULL)
                fatal_error(1, "can't open: '%s' (%s)",
                            argv[i], strerror(errno));
            
            /* initialize a lexer */
            Lexer lex = lexer_new(src, argv[i], R_FIRS);

            /*start the lexing */
            List tokens = cons_tokens(lex);
            
            /* check for error */
            if (tokens == NULL) {
                List errors = lexer_errors(lex);
                Token error;
                /* iterate over the error tokens list */
                while ((error = List_iter(errors)))
                    lex_error(src, error);
            } else {
                Token tok;
                /* iterate over the tokens list */
                while ((tok = List_iter(tokens)))
                    print_token(tok);
            }           
        }
    }  
    return 0;
}
