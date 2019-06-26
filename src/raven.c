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
#include "debug.h"
#include "error.h"
#include "eval.h"
#include "parser.h"
#include "lexer.h"
#include "resolver.h"
#include "token.h"

AST_piece parse(const char *src, const char *file, int *ident_num) {
    Lexer l;
    Parser p;

    init_lexer(&l, src, file);
    Token *tokens = cons_tokens(&l);

    if (lexer_error(&l)) {
        Err *errors = lexer_errors(&l);
        int errnum = lexer_errnum(&l);
        log_errs(errors, errnum, stderr);
        return NULL;
    }

    init_parser(&p, tokens);
    AST_piece piece = parse_piece(&p);

    if (parser_error(&p)) {
        Err *errors = parser_errors(&p);
        int errnum = parser_errnum(&p);
        log_errs(errors, errnum, stderr);
        return NULL;
    }

    if (ident_num != NULL)
        *ident_num = p.ident_num;

#ifdef PRINT_AST
    print_piece(piece);
#endif        

    return piece;
}

void run_line(const char *line, Resolver *r, Evaluator *e) {
    AST_piece piece = parse(line, "stdin", NULL);

    if (piece == NULL)
        return;

    if (resolve(r, piece)) {
        Err *errors = resolver_errors(r);
        int errnum = resolver_errnum(r);
        log_errs(errors, errnum, stderr);

        /* reset the resolver errors */
        r->been_error = 0;
        r->errors.len = 0;
        return;
    }

    //walk(e, piece);
}

int run_src(const char *src, const char *file) {
    /* number of AST_expr_ident in the source code, 
       used in the evaluator variable lookup table.*/
    int ident_num;
    AST_piece piece = parse(src, file, &ident_num);

    if (piece == NULL)
        return 1;

    Resolver r;
    Evaluator e;

    init_eval(&e, ident_num);
    init_resolver(&r, &e);
    
    if (resolve(&r, piece)) {
        Err *errors = resolver_errors(&r);
        int errnum = resolver_errnum(&r);
        log_errs(errors, errnum, stderr);
        return 1;
    }

    walk(&e, piece);

    free_resolver(&r);
    free_eval(&e);

    return 0;
}

#define MAX_LINE 1024   /* the maximum size for repl line */

void repl() {
    char buf[MAX_LINE];
    
    Resolver r;
    Evaluator e;

    /* resolver and evaluator are persist in the
       entire repl session, as all input lines
       share the same global environment. */
    init_resolver(&r, &e);
    init_eval(&e, 191);

    for (;;) {
        fputs("#> ", stdout);
        fgets(buf, MAX_LINE, stdin);
        run_line(buf, &r, &e);
    }

    free_resolver(&r);
    free_eval(&e);
}

int run_file(const char *file) {
    char *src = scan_file(file);
    
    if (src == NULL) {
        fatal_err(errno, "can't open: '%s' (%s)",
                  file, strerror(errno));
    }
    
    return run_src(src, file);
}

int main(int argc, char **argv) {
    
    for (int i = 1; i < argc; i++) {
        int err = run_file(argv[i]);
        if (err) break;
    }
    
    if (argc == 1)
        repl();
    
    return 0;
}
