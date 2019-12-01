/*
 * (test_lexer.c | 12 Feb 19 | Ahmad Maher)
 * 
 * lexer test cases.
 *
*/

#include <assert.h>
#include <stdio.h>  /* printf, puts */
#include <string.h> /* strcmp, strncmp */

#include "lexer.h"
#include "error.h"
#include "debug.h"
#include "token.h"
#include "strutil.h"

/* assert the equality of two tokens */
void assert_token(Token *t1, Token *t2) {
#ifdef PRINT_TOKENS
        print_token(t1);
        print_token(t2);
        puts("");
#endif
    assert(t1->type == t2->type);
    assert(t1->line == t2->line);
    assert(t1->length == t2->length);
    assert(!strcmp(t1->file, t2->file));
    assert(!strncmp(t1->lexeme, t2->lexeme, t1->length));
}

/* 
 * iterate over generated and expected tokens
 * asserting the equality of them.
*/
void assert_tokens(char *input, Token *expected, int num) {
    Lexer lex;
    init_lexer(&lex, input, "test");
    
    Token *tok = cons_tokens(&lex);
    assert(lexer_toknum(&lex) == num+1);
    
    for (int i = 0; tok[i].type != TK_EOF; i++)
        assert_token(&tok[i], &expected[i]);
}

/*
 * iterate over generated and expected lexing errors
 * asserting the equality of them.
 */
void assert_errors(char *input, Err *expected, int num) {
    Lexer lex;
    init_lexer(&lex, input, "test");

    cons_tokens(&lex);
    Err *errors = lexer_errors(&lex);
    int errnum = lexer_errnum(&lex);

    /* there has been an error */
    assert(lexer_error(&lex));
    /* same number of errors */
    assert(errnum == num);
    
    for (int i = 0; i < num; i++) {
        assert(expected[i].type == errors[i].type);
        assert(!strcmp(expected[i].message, errors[i].message));
        assert_token(&errors[i].where, &expected[i].where);
    }
}

#define TOKEN(type, lexeme)                                 \
    (Token){type, "test", lexeme, sizeof (lexeme) - 1, 1}

void test_literals() {
    char input[] = \
        "'foo' `bar` "
        "123 0xA1F 0b101 0o716 "
        "1.1 1e-10 "
        "true false "
        "nil";

    Token expected[] = {
        TOKEN(TK_STR, "'foo'"),
        TOKEN(TK_RSTR, "`bar`"),
        TOKEN(TK_INT, "123"),
        TOKEN(TK_INT, "0xA1F"),
        TOKEN(TK_INT, "0b101"),
        TOKEN(TK_INT, "0o716"),
        TOKEN(TK_FLOAT, "1.1"),
        TOKEN(TK_FLOAT, "1e-10"),
        TOKEN(TK_TRUE, "true"),
        TOKEN(TK_FALSE, "false"),
        TOKEN(TK_NIL, "nil")
    };

    int toknum = (sizeof expected) / (sizeof (Token));
    assert_tokens(input, expected, toknum);
    
    puts("literals\t==> passed");
}

void test_keywords() {
    char input[] = "fn return let do end if "
        "elif else for while continue break "
        "type cond match";

    Token expected[] = {
        TOKEN(TK_FN, "fn"),
        TOKEN(TK_RETURN, "return"),
        TOKEN(TK_LET, "let"),
        TOKEN(TK_DO, "do"),
        TOKEN(TK_END, "end"),
        TOKEN(TK_IF, "if"),
        TOKEN(TK_ELIF, "elif"),
        TOKEN(TK_ELSE, "else"),
        TOKEN(TK_FOR, "for"),
        TOKEN(TK_WHILE, "while"),
        TOKEN(TK_CONTINUE, "continue"),
        TOKEN(TK_BREAK, "break"),
        TOKEN(TK_TYPE, "type"),
        TOKEN(TK_COND, "cond"),
        TOKEN(TK_MATCH, "match"),
    };

    int toknum = (sizeof expected) / (sizeof (Token));
    assert_tokens(input, expected, toknum);
    
    puts("keywords\t==> passed");
}

void test_identifiers() {
    char input[] = "foo _bar_ id12 done";

    Token expected[] = {
        TOKEN(TK_IDENT, "foo"),
        TOKEN(TK_IDENT, "_bar_"),
        TOKEN(TK_IDENT, "id12"),
        TOKEN(TK_IDENT, "done")
    };
    
    int toknum = (sizeof expected) / (sizeof (Token));
    assert_tokens(input, expected, toknum);
    
    puts("identifiers\t==> passed");
}

void test_operators() {
    char input[] = "and or not . @"
        "+ - * / %"
        "< > == != <= >=";

    Token expected[] = {
        TOKEN(TK_AND, "and"),
        TOKEN(TK_OR, "or"),
        TOKEN(TK_NOT, "not"),
        TOKEN(TK_DOT, "."),
        TOKEN(TK_AT, "@"),
        TOKEN(TK_PLUS, "+"),
        TOKEN(TK_MINUS, "-"),
        TOKEN(TK_ASTERISK, "*"),
        TOKEN(TK_SLASH, "/"),
        TOKEN(TK_PERCENT, "%"),
        TOKEN(TK_LT, "<"),
        TOKEN(TK_GT, ">"),
        TOKEN(TK_EQ_EQ, "=="),
        TOKEN(TK_BANG_EQ, "!="),
        TOKEN(TK_LT_EQ, "<="),
        TOKEN(TK_GT_EQ, ">=")
    };

    
    int toknum = (sizeof expected) / (sizeof (Token));
    assert_tokens(input, expected, toknum);

    puts("operators\t==> passed");
}

void test_delimiters() {
    char input[] = "() {} [] , -> : ; = in | \n";

    Token expected[] = {
        TOKEN(TK_LPAREN, "("),
        TOKEN(TK_RPAREN, ")"),
        TOKEN(TK_LBRACE, "{"),
        TOKEN(TK_RBRACE, "}"),
        TOKEN(TK_LBRACKET, "["),
        TOKEN(TK_RBRACKET, "]"),
        TOKEN(TK_COMMA, ","),
        TOKEN(TK_DASH_GT, "->"),
        TOKEN(TK_COLON, ":"),
        TOKEN(TK_SEMICOLON, ";"),
        TOKEN(TK_EQ, "="),
        TOKEN(TK_IN, "in"),
        TOKEN(TK_PIPE, "|"),
        TOKEN(TK_NL, "")
    };
    
    int toknum = (sizeof expected) / (sizeof (Token));
    assert_tokens(input, expected, toknum);
    
    puts("delimiters\t==> passed");
}

void test_escaping() {
    /* the double slashes to prevent C compiler from
       escaping before the lexer does */
    char input[] =
        "'\\x51\\x75\\x6F\\x74\\x68'"
        "'\\x74\\x68\\x65\\x20\\x52\\x61\\x76\\x65\\x6E'"
        "'\\116\\145\\166\\145\\162\\155\\157\\162\\145'"
        "'\\a \\b \\f \\n \\r \\t \\v \\\\ \\' \\\"'";

    Token expected[] = {
        TOKEN(TK_STR, "'\\x51\\x75\\x6F\\x74\\x68'"),
        TOKEN(TK_STR, "'\\x74\\x68\\x65\\x20\\x52\\x61\\x76\\x65\\x6E'"),
        TOKEN(TK_STR, "'\\116\\145\\166\\145\\162\\155\\157\\162\\145'"),
        TOKEN(TK_STR, "'\\a \\b \\f \\n \\r \\t \\v \\\\ \\' \\\"'")
    };

    
    int toknum = (sizeof expected) / (sizeof (Token));
    assert_tokens(input, expected, toknum);

    puts("escaping\t==> passed");
}

void test_errors() {
    char input[] = "? 'inv\\z' 1e+ "
        "'\\777' '\\17' '\\888' "
        "'\\x1' '\\x1g' `unter";

    Token expected_toks[] = {
        TOKEN(TK_ERR, "?"),
        TOKEN(TK_ERR, "z'"),
        TOKEN(TK_ERR, "1e+"),
        TOKEN(TK_ERR, "777'"),
        TOKEN(TK_ERR, "17'"),
        TOKEN(TK_ERR, "888'"),
        TOKEN(TK_ERR, "1'"),
        TOKEN(TK_ERR, "1g'"),
        TOKEN(TK_ERR, "`unter")
    };

    Err expected_errs[] = {
        (Err) {SYNTAX_ERR, expected_toks[0], "unrecognize syntax"},
        (Err) {SYNTAX_ERR, expected_toks[1], "invalid escape sequence"},
        (Err) {SYNTAX_ERR, expected_toks[2], "malformed scientific notation"},
        (Err) {SYNTAX_ERR, expected_toks[3], "invalid escape sequence"},
        (Err) {SYNTAX_ERR, expected_toks[4], "invalid escape sequence"},
        (Err) {SYNTAX_ERR, expected_toks[5], "invalid escape sequence"},
        (Err) {SYNTAX_ERR, expected_toks[6], "invalid escape sequence"},
        (Err) {SYNTAX_ERR, expected_toks[7], "invalid escape sequence"},
        (Err) {SYNTAX_ERR, expected_toks[8], "unterminated string"}
    };

    int toknum = (sizeof expected_toks) / (sizeof (Token));
    assert_tokens(input, expected_toks, toknum);

    int errnum = (sizeof expected_errs) / (sizeof (Err));
    assert_errors(input, expected_errs, errnum);
    
    puts("errors\t\t==> passed");
}

/* test the inner functionality of the lexer */
void test_lexer() {
    test_literals();
    test_keywords();
    test_identifiers();
    test_delimiters();
    test_escaping();
    test_errors();
}

/**********************************************************/

#undef TOKEN
#define TOKEN(type, lexeme, line)                               \
    (Token){type, "test", lexeme, sizeof (lexeme) - 1, line}

/* test the layers which uses/used by the lexer
   (e.g. scanfile, cons_tokens, list iteration ...) */
void test_file_lexing() {
    Token expected_toks[] = {
        TOKEN(TK_NL, "", 2), 
        TOKEN(TK_NL, "", 24),
        TOKEN(TK_NL, "", 25),
        TOKEN(TK_FN, "fn", 26),
        TOKEN(TK_IDENT, "derivative", 26),
        TOKEN(TK_LPAREN, "(", 26),
        TOKEN(TK_IDENT, "f", 26),
        TOKEN(TK_RPAREN, ")", 26),
        TOKEN(TK_NL, "", 26),
        TOKEN(TK_LET, "let", 27),
        TOKEN(TK_IDENT, "delta", 27),
        TOKEN(TK_EQ, "=", 27),
        TOKEN(TK_FLOAT, "1e-4", 27),
        TOKEN(TK_NL, "", 27),
        TOKEN(TK_RETURN, "return", 28),
        TOKEN(TK_FN, "fn", 28),
        TOKEN(TK_LPAREN, "(", 28),
        TOKEN(TK_IDENT, "n", 28),
        TOKEN(TK_RPAREN, ")", 28),
        TOKEN(TK_NL, "", 28),
        TOKEN(TK_LPAREN, "(", 29),
        TOKEN(TK_IDENT, "f", 29),
        TOKEN(TK_LPAREN, "(", 29),
        TOKEN(TK_IDENT, "n", 29),
        TOKEN(TK_PLUS, "+", 29),
        TOKEN(TK_IDENT, "delta", 29),
        TOKEN(TK_RPAREN, ")", 29),
        TOKEN(TK_MINUS, "-", 29),
        TOKEN(TK_IDENT, "f", 29),
        TOKEN(TK_LPAREN, "(", 29),
        TOKEN(TK_IDENT, "n", 29),
        TOKEN(TK_RPAREN, ")", 29),
        TOKEN(TK_RPAREN, ")", 29),
        TOKEN(TK_SLASH, "/", 29),
        TOKEN(TK_IDENT,"delta", 29),
        TOKEN(TK_NL, "", 29),
        TOKEN(TK_END, "end", 30),
        TOKEN(TK_NL, "", 30),
        TOKEN(TK_END, "end", 31),
        TOKEN(TK_NL, "", 31),
        TOKEN(TK_NL, "", 32),
        TOKEN(TK_ERR, "?", 33),
        TOKEN(TK_ERR, "`unterminate", 33),
    };
    
    Err expected_errs[] = {
        (Err){SYNTAX_ERR, expected_toks[41], "unrecognize syntax"},
        (Err){SYNTAX_ERR, expected_toks[42], "unterminated string"}
    };
    
    /* 
       NOTE: 
       run the test from the main directory,
       not from the /bin directory.
    */
    
    char *path = "tests/test_lexer.rav";
    char *buf = scan_file(path);
    assert(buf != NULL);

    int toknum = (sizeof expected_toks) / (sizeof (Token));
    assert_tokens(buf, expected_toks, toknum);
    int errnum = (sizeof expected_errs) / (sizeof (Err));
    assert_errors(buf, expected_errs, errnum);

    puts("file\t\t==> passed");
}

int main(void) {
    puts("internal testing ...");
    test_lexer();
    puts("layers testing ...");
    test_file_lexing();
    return 0;
}
