/*
 *
 * (test_lexer.c | 10 Dec 18 | Amr Anwar, Kareem Hamdy)
 * 
 * test for the lexer.
 *
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../src/alloc.h"
#include "../src/lexer.h"
#include "../src/error.h"
#include "../src/list.h"

#undef PRINT_TOKENS

/* assert the equality of two tokens */
#define assert_token(t1, t2)                                            \
    assert(t1.type == t2.type);                                         \
    assert(t1.line == t2.line);                                         \
    assert(!strcmp(t1.file, t2.file));                                  \
    if ((t1.lexeme == NULL) || (t2.lexeme == NULL))                     \
        assert((t1.lexeme == NULL) && (t2.lexeme == NULL));             \
    else                                                                \
        assert(!strcmp(t1.lexeme, t2.lexeme));                          \


/* iterate over generated and expected tokens
   asserting the equality of them */
void assert_tokens(lexer *lex, char *input, token *expected, char *name) {
    init_lexer(lex, input, name);
    
    token tok = cons_token(lex);
    for (int i = 0; tok.type != TK_EOF; i++) {
#ifdef PRINT_TOKENS
        print_token(alloc_token(tok, R_SECN));
        print_token(alloc_token(expected[i], R_SECN));
        puts("");
#endif
        assert_token(tok, expected[i]);
        tok = cons_token(lex);
    }
}

void test_literals(lexer *lex) {
    char input[] = \
        "'foo' `bar` "
        "1 0xA1F 0b101 0o716\n"
        "1.1 1e-10 "
        "true false\n"
        "nil";

    token expected[] = {
        {TK_STR, "foo", "literals", 1},
        {TK_RSTR, "bar", "literals", 1},
        {TK_INT, "1", "literals", 1},
        {TK_INT, "0xA1F", "literals", 1},
        {TK_INT, "0b101", "literals", 1},
        {TK_INT, "0o716", "literals", 1},
        {TK_NL, NULL, "literals", 1},
        {TK_FLOAT, "1.1", "literals", 2},
        {TK_FLOAT, "1e-10", "literals", 2},
        {TK_TRUE, NULL, "literals", 2},
        {TK_FALSE, NULL, "literals", 2},
        {TK_NL, NULL, "literals", 2},
        {TK_NIL, NULL, "literals", 3}
    };

    assert_tokens(lex, input, expected, "literals");
    puts("literals\t==> passed");
}

void test_keywords(lexer *lex) {
    char input[] = "fn return let do end if\n"
        "elif else for while continue break\n"
        "match case";

    token expected[] = {
        {TK_FN, NULL, "keywords", 1},
        {TK_RETURN, NULL, "keywords", 1},
        {TK_LET, NULL, "keywords", 1},
        {TK_DO, NULL, "keywords", 1},
        {TK_END, NULL, "keywords", 1},
        {TK_IF, NULL, "keywords", 1},
        {TK_NL, NULL, "keywords", 1},
        {TK_ELIF, NULL, "keywords", 2},
        {TK_ELSE, NULL, "keywords", 2},
        {TK_FOR, NULL, "keywords", 2},
        {TK_WHILE, NULL, "keywords", 2},
        {TK_CONTINUE, NULL, "keywords", 2},
        {TK_BREAK, NULL, "keywords", 2},
        {TK_NL, NULL, "keywords", 2},
        {TK_MATCH, NULL, "keywords", 3},
        {TK_CASE, NULL, "keywords", 3}
    };

    assert_tokens(lex, input, expected, "keywords");
    puts("keywords\t==> passed");
}

void test_identifiers(lexer *lex) {
    char input[] = "foo _bar id12 _un_";

    token expected[] = {
        {TK_IDENT, "foo", "identifiers", 1},
        {TK_IDENT, "_bar", "identifiers", 1},
        {TK_IDENT, "id12", "identifiers", 1},
        {TK_IDENT, "_un_", "identifiers", 1}
    };

    assert_tokens(lex, input, expected, "identifiers");
    puts("identifiers\t==> passed");
}

void test_operators(lexer *lex) {
    char input[] = "and or not . @\n"
        "+ - * / %\n"
        "< > == != <= >=\n"
        "| & ^ ~ >> <<";

    token expected[] = {
        {TK_AND, NULL, "operators", 1},
        {TK_OR, NULL, "operators", 1},
        {TK_NOT, NULL, "operators", 1},
        {TK_DOT, NULL, "operators", 1},
        {TK_AT, NULL, "operators", 1},
        {TK_NL, NULL, "operators", 1},
        {TK_PLUS, NULL, "operators", 2},
        {TK_MINUS, NULL, "operators", 2},
        {TK_ASTERISK, NULL, "operators", 2},
        {TK_SLASH, NULL, "operators", 2},
        {TK_PERCENT, NULL, "operators", 2},
        {TK_NL, NULL, "operators", 2},
        {TK_LT, NULL, "operators", 3},
        {TK_GT, NULL, "operators", 3},
        {TK_EQ_EQ, NULL, "operators", 3},
        {TK_BANG_EQ, NULL, "operators", 3},
        {TK_LT_EQ, NULL, "operators", 3},
        {TK_GT_EQ, NULL, "operators", 3},
        {TK_NL, NULL, "operators", 3},
        {TK_PIPE, NULL, "operators", 4},
        {TK_AMPERSAND, NULL, "operators", 4},
        {TK_CARET, NULL, "operators", 4},
        {TK_TILDE, NULL, "operators", 4},
        {TK_GT_GT, NULL, "operators", 4},
        {TK_LT_LT, NULL, "operators", 4}
    };

    assert_tokens(lex, input, expected, "operators");
    puts("operators\t==> passed");
}

void test_delimiters(lexer *lex) {
    char input[] = "() {} [] , -> : = in \n";

    token expected[] = {
        {TK_LPAREN, NULL, "delimiters", 1},
        {TK_RPAREN, NULL, "delimiters", 1},
        {TK_LBRACE, NULL, "delimiters", 1},
        {TK_RBRACE, NULL, "delimiters", 1},
        {TK_LBRACKET, NULL, "delimiters", 1},
        {TK_RBRACKET, NULL, "delimiters", 1},
        {TK_COMMA, NULL, "delimiters", 1},
        {TK_DASH_GT, NULL, "delimiters", 1},
        {TK_COLON, NULL, "delimiters", 1},
        {TK_EQ, NULL, "delimiters", 1},
        {TK_IN, NULL, "delimiters", 1},
        {TK_NL, NULL, "delimiters", 1}
    };

    assert_tokens(lex, input, expected, "delimiters");
    puts("delimiterst\t==> passed");
}

void test_escaping(lexer *lex) {
    /* the double slashes to prevent C compiler from
       escaping before the lexer does */
    char input[] =
        "'\\x51\\x75\\x6F\\x74\\x68'"
        "'\\x74\\x68\\x65\\x20\\x52\\x61\\x76\\x65\\x6E'"
        "'\\116\\145\\166\\145\\162\\155\\157\\162\\145'"
        "'\\a \\b \\f \\n \\r \\t \\v \\\\ \\' \\\"'";

    token expected[] = {
        {TK_STR, "Quoth", "escaping", 1},
        {TK_STR, "the Raven", "escaping", 1},
        {TK_STR, "Nevermore", "escaping", 1},
        {TK_STR, "\a \b \f \n \r \t \v \\ \' \"", "escaping", 1}
    };

    assert_tokens(lex, input, expected, "escaping");
    puts("escapingt\t==> passed");
}

void test_errors(lexer *lex) {
    char input[] = "? 'inv\\z' 1e+ \n"
        "'\\777' '\\17' '\\888'\n"
        "'\\x1' '\\x1g' `unter";

    token expected[] = {
        {TK_UNRECOG, "?", "errors", 1},
        {TK_INVALID_ESCP, "'inv\\z'", "errors", 1},
        {TK_INVALID_SCIEN, "1e+", "errors", 1},
        {TK_NL, NULL, "errors", 1},
        {TK_OCT_OUTOFR_ESCP, "'\\777'", "errors", 2},
        {TK_OCT_MISS_ESCP, "'\\17'", "errors", 2},
        {TK_OCT_INVL_ESCP, "'\\888'", "errors", 2},
        {TK_NL, NULL, "errors", 2},
        {TK_HEX_MISS_ESCP, "'\\x1'", "errors", 3},
        {TK_HEX_INVL_ESCP, "'\\x1g'", "errors", 3},
        {TK_UNTERMIN_STR, "`unter", "errors", 3},
    };

    assert_tokens(lex, input, expected, "errors");
    puts("errors\t\t==> passed");
}

/* test the inner functionality of the lexer */
void test_lexer() {
    lexer *lex = make(lex, R_SECN);
    
    test_literals(lex);
    test_keywords(lex);
    test_identifiers(lex);
    test_delimiters(lex);
    test_escaping(lex);
    test_errors(lex);
}

/* test the layers which uses/used by the lexer
   (e.g. scanfile, cons_tokens, list iteration ...) */
void test_file_lexing() {
    token tok_expected[] = {
        {TK_NL, NULL, "test_lexer", 2},
        {TK_NL, NULL, "test_lexer", 24},
        {TK_NL, NULL, "test_lexer", 25},
        {TK_FN, NULL, "test_lexer", 26},
        {TK_IDENT, "derivative", "test_lexer", 26},
        {TK_LPAREN, NULL, "test_lexer", 26},
        {TK_IDENT, "f", "test_lexer", 26},
        {TK_RPAREN, NULL, "test_lexer", 26},
        {TK_NL, NULL, "test_lexer", 26},
        {TK_LET, NULL, "test_lexer", 27},
        {TK_IDENT, "delta", "test_lexer", 27},
        {TK_EQ, NULL, "test_lexer", 27},
        {TK_FLOAT, "1e-4", "test_lexer", 27},
        {TK_NL, NULL, "test_lexer", 27},
        {TK_RETURN, NULL, "test_lexer", 28},
        {TK_FN, NULL, "test_lexer", 28},
        {TK_LPAREN, NULL, "test_lexer", 28},
        {TK_IDENT, "n", "test_lexer", 28},
        {TK_RPAREN, NULL, "test_lexer", 28},
        {TK_NL, NULL, "test_lexer", 28},
        {TK_LPAREN, NULL, "test_lexer", 29},
        {TK_IDENT, "f", "test_lexer", 29},
        {TK_LPAREN, NULL, "test_lexer", 29},
        {TK_IDENT, "n", "test_lexer", 29},
        {TK_PLUS, NULL, "test_lexer", 29},
        {TK_IDENT, "delta", "test_lexer", 29},
        {TK_RPAREN, NULL, "test_lexer", 29},
        {TK_MINUS, NULL, "test_lexer", 29},
        {TK_IDENT, "f", "test_lexer", 29},
        {TK_LPAREN, NULL, "test_lexer", 29},
        {TK_IDENT, "n", "test_lexer", 29},
        {TK_RPAREN, NULL, "test_lexer", 29},
        {TK_RPAREN, NULL, "test_lexer", 29},
        {TK_SLASH, NULL, "test_lexer", 29},
        {TK_IDENT,"delta", "test_lexer", 29},
        {TK_NL, NULL, "test_lexer", 29},
        {TK_END, NULL, "test_lexer", 30},
        {TK_NL, NULL, "test_lexer", 30},
        {TK_END, NULL, "test_lexer", 31},
        {TK_NL, NULL, "test_lexer", 31},
        {TK_NL, NULL, "test_lexer", 32},
        {TK_EOF, NULL, "test_lexer", 33}
    };

    token err_expected[] = {
        {TK_UNRECOG, "?", "test_lexer", 33},
        {TK_UNTERMIN_STR, "\"unterminate", "test_lexer", 33},
    };

    char *path = "../tests/test_lexer.rav";
    char *buf = scan_file(path);
    assert(buf != NULL);
    
    lexer *lex = make(lex, R_SECN);
    assert(lex != NULL);
    init_lexer(lex, buf, "test_lexer");
    
    token_list *tok_lst = cons_tokens(lex);
    list *toks = tok_lst->tokens;
    list *errs = tok_lst->error_tokens;

    /* there are lexing errors in the test file */
    assert(tok_lst->been_error);

    /* assert non-error tokens */
    token *tok = iter_list(toks);
    for (int i = 0; tok != NULL; i++) {
#ifdef PRINT_TOKENS
        print_token(tok);
        print_token(alloc_token(tok_expected[i], R_SECN));
        puts("");
#endif
        assert_token((*tok), tok_expected[i]);
        tok = iter_list(toks);
    }

    /* assert error tokens */
    tok = iter_list(errs);
    for (int i = 0; tok != NULL; i++) {
#ifdef PRINT_TOKENS
        print_token(tok);
        print_token(alloc_token(err_expected[i], R_SECN));
        puts("");
#endif
        assert_token((*tok), err_expected[i]);
        tok = iter_list(errs);
    }

    puts("file\t\t==> passed");
}

int main(void) {
    puts("internal testing ...");
    test_lexer();
    puts("layers testing ...");
    test_file_lexing();
    return 0;
}
