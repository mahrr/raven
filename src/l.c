/*
 * (l.c | 28 Nov 18 | Ahmad Maher)
 *
 * The main entry for the interpreter.
 *
*/

#include <stdio.h>

#include "lexer.h"
#include "alloc.h"
#include "error.h"

/* @@ just print tokens for now */

#define MAX_LINE 1024   /* the maximum size for repl line */

void repl() {
    char buf[MAX_LINE];
    token tok;
    
    for (;;) {
        fputs("#> ", stdout);
        fgets(buf, MAX_LINE, stdin);
        init_lexer(buf, "stdin");
        
        tok = cons_token();
        while (tok.type != EOF_TOK) {
            
            if (is_errtok(&tok))
                lex_error(&tok);
            else
                print_token(&tok);
            tok = cons_token();
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
            /*start the lexing */
            init_lexer(src, argv[i]);
            token_list *tl = cons_tokens();
            
            list *toks;
            token *tok;
            if (tl->been_error) {
                toks = tl->error_tokens;
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
