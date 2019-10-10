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
#include "hashing.h"
#include "lexer.h"
#include "parser.h"
#include "resolver.h"
#include "token.h"

AST_piece parse(const char *src, const char *file, Lexer *l, Parser *p) {
    init_lexer(l, src, file);
    Token *tokens = cons_tokens(l);

    if (lexer_error(l)) {
        Err *errors = lexer_errors(l);
        int errnum = lexer_errnum(l);
        log_errs(errors, errnum, stderr);
        return NULL;
    }

    init_parser(p, tokens);
    AST_piece piece = parse_piece(p);

    if (parser_error(p)) {
        Err *errors = parser_errors(p);
        int errnum = parser_errnum(p);
        log_errs(errors, errnum, stderr);
        return NULL;
    }

#ifdef PRINT_AST
    print_piece(piece);
    putchar('\n');
#endif

    return piece;
}

void run_line(const char *line, Resolver *r, Evaluator *e) {
    Parser p;
    Lexer l;
    
    AST_piece piece = parse(line, "stdin", &l, &p);

    if (piece == NULL)
        return;

    for (int i = 0; piece->stmts[i]; i++) {
        if (resolve_statement(r, piece->stmts[i])) {
            Err *errors = resolver_errors(r);
            int errnum = resolver_errnum(r);
            log_errs(errors, errnum, stderr);
        
            /* reset the resolver errors */
            r->been_error = 0;
            r->errors.len = 0;
            return;
        }

        Rav_obj *result = execute(e, piece->stmts[i]);
        if (result->type != VOID_OBJ) {
            printf("=> ");
            echo_object(result);
            putchar('\n');
        }
    }

    free_lexer(&l);
    free_parser(&p);
}

int run_src(const char *src, const char *file) {
    Lexer l;
    Parser p;
    
    AST_piece piece = parse(src, file, &l, &p);

    if (piece == NULL)
        return 1;

    Resolver r;
    Evaluator e;

    int ident_num = parser_idnum(&p);
    /* use prime number for the table size. */
    init_eval(&e, next_prime(ident_num));
    init_resolver(&r, &e);

    if (resolve(&r, piece)) {
        Err *errors = resolver_errors(&r);
        int errnum = resolver_errnum(&r);
        log_errs(errors, errnum, stderr);
        return 1;
    }

    walk(&e, piece);

    free_lexer(&l);
    free_parser(&p);
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
        fputs(">> ", stdout);
        if (!fgets(buf, MAX_LINE, stdin))
            break;
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
