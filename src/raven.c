/*
 * (l.c | 28 Nov 18 | Ahmad Maher)
 *
 * The main entry for the interpreter.
 *
*/

#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "alloc.h"
#include "error.h"

/* @@ just print tokens for now */

#define MAX_LINE 1024   /* the maximum size for repl line */

void repl() {
    char buf[MAX_LINE];
    token tok;
    lexer *lex = make(lex, R_FIRS);
    
    for (;;) {
        fputs("#> ", stdout);
        fgets(buf, MAX_LINE, stdin);
        init_lexer(lex, buf, "stdin");
        
        tok = cons_token(lex);
        while (tok.type != TK_EOF) {
            
            if (is_errtok(&tok))
                lex_error(&tok);
            else
                print_token(&tok);
            tok = cons_token(lex);
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
            if (src == NULL) {
                fprintf(stderr, "No such file or directory: %s", argv[i]);
                exit(1);
            }
            
            /* initialize a lexer */
            lexer *lex = make(lex, R_FIRS);
            init_lexer(lex, src, argv[i]);
            /*start the lexing */
            token_list *tl = cons_tokens(lex);
            
            list *toks;
            token *tok;
            if (tl->been_error) {
                toks = tl->error_tokens;
                /* iterate over the error tokens list */
                while (tok = iter_list(toks))
                    lex_error(tok);
            } else {
                toks = tl->tokens;
                /* iterate over the tokens list */
                while (tok = iter_list(toks))
                    print_token(tok);
            }           
        }
    }  
    return 0;
}
