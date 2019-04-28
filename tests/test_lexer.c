/*
 * (test_lexer.c | 12 Feb 19 | Ahmad Maher)
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
#include "../src/debug.h"
#include "../src/list.h"

#undef PRINT_TOKENS

/* assert the equality of two tokens */
#define assert_token(t1, t2)                                       \
    assert(t1.type == t2.type);                                    \
    assert(t1.line == t2.line);                                    \
    assert(t1.length == t2.length);                                \
    assert(!strcmp(t1.file, t2.file));                             \
    if ((t1.lexeme == NULL) || (t2.lexeme == NULL))                \
        assert((t1.lexeme == NULL) && (t2.lexeme == NULL));        \
    else                                                           \
        assert(!strncmp(t1.lexeme, t2.lexeme, strlen(t2.lexeme))); \
    if (t1.type == TK_ERR)                                         \
        assert(!strcmp(t1.err_msg, t2.err_msg));                   \


/* iterate over generated and expected tokens
   asserting the equality of them */
void
assert_tokens(lexer *lex, char *input, token *expected, char *name) {
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

/* tokens with lexemes (e.g identifiers, literals) */
#define A_TOKEN(t, s) {t, s, (sizeof s) - 1, "test", 1, NULL}

/* error tokens with error messages */
#define E_TOKEN(s, m) {TK_ERR, s, (sizeof s) - 1, "test", 1, m}

/* tokesn without lexemes (e.g operators, keywords) */
#define N_TOKEN(t) {t, NULL, 0, "test", 1, NULL} 

void test_literals(lexer *lex) {
    char input[] = \
        "'foo' `bar` "
        "123 0xA1F 0b101 0o716 "
        "1.1 1e-10 "
        "true false "
        "nil";

    token expected[] = {
        A_TOKEN(TK_STR, "'foo'"),
        A_TOKEN(TK_STR, "`bar`"),
        A_TOKEN(TK_INT, "123"),
        A_TOKEN(TK_INT, "0xA1F"),
        A_TOKEN(TK_INT, "0b101"),
        A_TOKEN(TK_INT, "0o716"),
        A_TOKEN(TK_FLOAT, "1.1"),
        A_TOKEN(TK_FLOAT, "1e-10"),
        N_TOKEN(TK_TRUE),
        N_TOKEN(TK_FALSE),
        N_TOKEN(TK_NIL)
    };

    assert_tokens(lex, input, expected, "test");
    puts("literals\t==> passed");
}

void test_keywords(lexer *lex) {
    char input[] = "fn return let do end if "
        "elif else for while continue break "
        "match case";

    token expected[] = {
        N_TOKEN(TK_FN),
        N_TOKEN(TK_RETURN),
        N_TOKEN(TK_LET),
        N_TOKEN(TK_DO),
        N_TOKEN(TK_END),
        N_TOKEN(TK_IF),
        N_TOKEN(TK_ELIF),
        N_TOKEN(TK_ELSE),
        N_TOKEN(TK_FOR),
        N_TOKEN(TK_WHILE),
        N_TOKEN(TK_CONTINUE),
        N_TOKEN(TK_BREAK),
        N_TOKEN(TK_MATCH),
        N_TOKEN(TK_CASE)
    };

    assert_tokens(lex, input, expected, "test");
    puts("keywords\t==> passed");
}

void test_identifiers(lexer *lex) {
    char input[] = "foo _bar_ id12 done";

    token expected[] = {
        A_TOKEN(TK_IDENT, "foo"),
        A_TOKEN(TK_IDENT, "_bar_"),
        A_TOKEN(TK_IDENT, "id12"),
        A_TOKEN(TK_IDENT, "done")
    };

    assert_tokens(lex, input, expected, "test");
    puts("identifiers\t==> passed");
}

void test_operators(lexer *lex) {
    char input[] = "and or not . @"
        "+ - * / % |"
        "< > == != <= >=";

    token expected[] = {
        N_TOKEN(TK_AND),
        N_TOKEN(TK_OR),
        N_TOKEN(TK_NOT),
        N_TOKEN(TK_DOT),
        N_TOKEN(TK_AT),
        N_TOKEN(TK_PLUS),
        N_TOKEN(TK_MINUS),
        N_TOKEN(TK_ASTERISK),
        N_TOKEN(TK_SLASH),
        N_TOKEN(TK_PERCENT),
        N_TOKEN(TK_PIPE),
        N_TOKEN(TK_LT),
        N_TOKEN(TK_GT),
        N_TOKEN(TK_EQ_EQ),
        N_TOKEN(TK_BANG_EQ),
        N_TOKEN(TK_LT_EQ),
        N_TOKEN(TK_GT_EQ)
    };

    assert_tokens(lex, input, expected, "test");
    puts("operators\t==> passed");
}

void test_delimiters(lexer *lex) {
    char input[] = "() {} [] , -> : ; = in \n";

    token expected[] = {
        N_TOKEN(TK_LPAREN),
        N_TOKEN(TK_RPAREN),
        N_TOKEN(TK_LBRACE),
        N_TOKEN(TK_RBRACE),
        N_TOKEN(TK_LBRACKET),
        N_TOKEN(TK_RBRACKET),
        N_TOKEN(TK_COMMA),
        N_TOKEN(TK_DASH_GT),
        N_TOKEN(TK_COLON),
        N_TOKEN(TK_SEMICOLON),
        N_TOKEN(TK_EQ),
        N_TOKEN(TK_IN),
        N_TOKEN(TK_NL)
    };

    assert_tokens(lex, input, expected, "test");
    puts("delimiters\t==> passed");
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
        A_TOKEN(TK_STR, "'Quoth'"),
        A_TOKEN(TK_STR, "'the Raven'"),
        A_TOKEN(TK_STR, "'Nevermore'"),
        A_TOKEN(TK_STR, "'\a \b \f \n \r \t \v \\ \' \"'")
    };

    assert_tokens(lex, input, expected, "test");
    puts("escaping\t==> passed");
}

void test_errors(lexer *lex) {
    char input[] = "? 'inv\\z' 1e+ "
        "'\\777' '\\17' '\\888' "
        "'\\x1' '\\x1g' `unter";

    token expected[] = {
        E_TOKEN("?", "unrecognize syntax"),
        E_TOKEN("z'", "invalid escape sequence"),
        E_TOKEN("1e+", "malformed scientific notation"),
        E_TOKEN("777'", "invalid escape sequence"),
        E_TOKEN("17'", "invalid escape sequence"),
        E_TOKEN("888'", "invalid escape sequence"),
        E_TOKEN("1'", "invalid escape sequence"),
        E_TOKEN("1g'", "invalid escape sequence"),
        E_TOKEN("`unter", "unterminated string")
    };

    assert_tokens(lex, input, expected, "test");
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

/**********************************************************/

/* like the macros above but with the token line unspecified */
#define A_FTOKEN(t, s, l) {t, s, (sizeof s) - 1, "test_lexer", l, NULL}
#define E_FTOKEN(s, l, m) {TK_ERR, s, (sizeof s) - 1, "test_lexer", l, m}
#define N_FTOKEN(t, l)    {t, NULL, 0, "test_lexer", l, NULL} 

/* test the layers which uses/used by the lexer
   (e.g. scanfile, cons_tokens, list iteration ...) */
void test_file_lexing() {
    token tok_expected[] = {
        N_FTOKEN(TK_NL, 2), 
        N_FTOKEN(TK_NL, 24),
        N_FTOKEN(TK_NL, 25),
        N_FTOKEN(TK_FN, 26),
        A_FTOKEN(TK_IDENT, "derivative", 26),
        N_FTOKEN(TK_LPAREN, 26),
        A_FTOKEN(TK_IDENT, "f", 26),
        N_FTOKEN(TK_RPAREN, 26),
        N_FTOKEN(TK_NL, 26),
        N_FTOKEN(TK_LET, 27),
        A_FTOKEN(TK_IDENT, "delta", 27),
        N_FTOKEN(TK_EQ, 27),
        A_FTOKEN(TK_FLOAT, "1e-4", 27),
        N_FTOKEN(TK_NL, 27),
        N_FTOKEN(TK_RETURN, 28),
        N_FTOKEN(TK_FN, 28),
        N_FTOKEN(TK_LPAREN, 28),
        A_FTOKEN(TK_IDENT, "n", 28),
        N_FTOKEN(TK_RPAREN, 28),
        N_FTOKEN(TK_NL, 28),
        N_FTOKEN(TK_LPAREN, 29),
        A_FTOKEN(TK_IDENT, "f", 29),
        N_FTOKEN(TK_LPAREN, 29),
        A_FTOKEN(TK_IDENT, "n", 29),
        N_FTOKEN(TK_PLUS, 29),
        A_FTOKEN(TK_IDENT, "delta", 29),
        N_FTOKEN(TK_RPAREN, 29),
        N_FTOKEN(TK_MINUS, 29),
        A_FTOKEN(TK_IDENT, "f", 29),
        N_FTOKEN(TK_LPAREN, 29),
        A_FTOKEN(TK_IDENT, "n", 29),
        N_FTOKEN(TK_RPAREN, 29),
        N_FTOKEN(TK_RPAREN, 29),
        N_FTOKEN(TK_SLASH, 29),
        A_FTOKEN(TK_IDENT,"delta", 29),
        N_FTOKEN(TK_NL, 29),
        N_FTOKEN(TK_END, 30),
        N_FTOKEN(TK_NL, 30),
        N_FTOKEN(TK_END, 31),
        N_FTOKEN(TK_NL, 31),
        N_FTOKEN(TK_NL, 32),
        N_FTOKEN(TK_EOF, 33)
    };

    token err_expected[] = {
        E_FTOKEN("?", 33, "unrecognize syntax"),
        E_FTOKEN("`unterminate", 33, "unterminated string") 
    };

    /* 
       NOTE: 
       run the test from the main directory,
       not from the /bin directory.
    */
    
    char *path = "tests/test_lexer.rav";
    char *buf = scan_file(path);
    assert(buf != NULL);
    
    lexer *lex = make(lex, R_SECN);
    assert(lex != NULL);
    init_lexer(lex, buf, "test_lexer");
    
    token_list *tok_lst = cons_tokens(lex);
    List_T toks = tok_lst->tokens;
    List_T errs = tok_lst->error_tokens;

    /* there are lexing errors in the test file */
    assert(tok_lst->been_error);

    /* assert non-error tokens */
    token *tok = List_iter(toks);
    for (int i = 0; tok != NULL; i++) {
#ifdef PRINT_TOKENS
        print_token(tok);
        print_token(alloc_token(tok_expected[i], R_SECN));
        puts("");
#endif
        assert_token((*tok), tok_expected[i]);
        tok = List_iter(toks);
    }

    /* assert error tokens */
    tok = List_iter(errs);
    for (int i = 0; tok != NULL; i++) {
#ifdef PRINT_TOKENS
        print_token(tok);
        print_token(alloc_token(err_expected[i], R_SECN));
        puts("");
#endif
        assert_token((*tok), err_expected[i]);
        tok = List_iter(errs);
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
