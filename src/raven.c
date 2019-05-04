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

#include "ast.h"
#include "alloc.h"
#include "lexer.h"
#include "parser.h"
#include "error.h"
#include "debug.h"

/* @@ just print tokens for now */

#define MAX_LINE 1024   /* the maximum size for repl line */

void repl() {
    char buf[MAX_LINE];
    Lexer l = lexer_new("", "", R_FIRS);
    Parser p = parser_new(List_new(R_FIRS), R_FIRS);

    for (;;) {
        fputs("#> ", stdout);
        fgets(buf, MAX_LINE, stdin);

        init_lexer(l, buf, "stdin");
        List tokens = cons_tokens(l);

        /* lexing error occur */
        if (tokens == NULL) {
            List errors = lexer_errors(l);
            Token error;

            while ((error = (Token)List_iter(errors)))
                lex_error(buf, error);
        }
        
        init_parser(p, tokens);
        AST_piece piece = parse_piece(p);

        /* parsing error occur */
        if (parser_error(p))
            parser_log(p, stdout);
        else
            print_piece(piece);
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
            
            /* lexing error */
            if (tokens == NULL) {
                List errors = lexer_errors(lex);
                Token error;
                /* iterate over the error tokens list */
                while ((error = List_iter(errors)))
                    lex_error(src, error);

                return 1;
            }

            Parser parser = parser_new(tokens, R_FIRS);
            AST_piece piece = parse_piece(parser);

            /* parsing error */
            if (parser_error(parser)) {
                parser_log(parser, stdout);
                return 1;
            }

            print_piece(piece);
        }
    }  
    return 0;
}
