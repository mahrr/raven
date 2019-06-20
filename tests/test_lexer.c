/*
 * (test_lexer.c | 12 Feb 19 | Ahmad Maher)
 * 
 * test for the lexer.
 *
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "alloc.h"
#include "lexer.h"
#include "error.h"
#include "debug.h"
#include "list.h"

/* assert the equality of two tokens */
void assert_token(Token t1, Token t2) {
    assert(t1->type == t2->type);
    assert(t1->line == t2->line);
    assert(t1->length == t2->length);
    assert(!strcmp(t1->file, t2->file));
    assert(!strncmp(t1->lexeme, t2->lexeme, strlen(t2->lexeme)));
    if (t1->type == TK_ERR)
        assert(!strcmp(t1->err_msg, t2->err_msg));
}

/* iterate over generated and expected tokens
   asserting the equality of them */
void
assert_tokens(Lexer lex, char *input, Token *expected, char *name) {
    init_lexer(lex, input, name);
    
    Token tok = cons_token(lex);
    for (int i = 0; tok->type != TK_EOF; i++) {
#ifdef PRINT_TOKENS
        print_token(tok);
        print_token(expected[i]);
        puts("");
#endif
        assert_token(tok, expected[i]);
        tok = cons_token(lex);
    }
}

/* tokens with lexemes (e.g identifiers, literals) */
Token NEW_TOKEN(TK_type type, char *lexeme, long line) {
    Token tok = make(tok, R_FIRS);
    tok->type = type;
    tok->lexeme = lexeme;
    tok->length = strlen(lexeme);
    tok->file = "test";
    tok->line = line;
    tok->err_msg = NULL;

    return tok;
}

/* error tokens with error messages */
Token NEW_ERROR(char *lexeme, char *msg, long line) {
    Token err = NEW_TOKEN(TK_ERR, lexeme, line);
    err->err_msg = msg;

    return err;
}

#define TOKEN(type, lexeme) NEW_TOKEN(type, lexeme, 1)
#define ERROR(lexeme, msg)  NEW_ERROR(lexeme, msg, 1)

void test_literals(Lexer lex) {
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

    assert_tokens(lex, input, expected, "test");
    puts("literals\t==> passed");
}

void test_keywords(Lexer lex) {
    char input[] = "fn return let do end if "
        "elif else for while continue break "
        "match case";

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
        TOKEN(TK_MATCH, "match"),
        TOKEN(TK_CASE, "case")
    };

    assert_tokens(lex, input, expected, "test");
    puts("keywords\t==> passed");
}

void test_identifiers(Lexer lex) {
    char input[] = "foo _bar_ id12 done";

    Token expected[] = {
        TOKEN(TK_IDENT, "foo"),
        TOKEN(TK_IDENT, "_bar_"),
        TOKEN(TK_IDENT, "id12"),
        TOKEN(TK_IDENT, "done")
    };

    assert_tokens(lex, input, expected, "test");
    puts("identifiers\t==> passed");
}

void test_operators(Lexer lex) {
    char input[] = "and or not . @"
        "+ - * / % |"
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
        TOKEN(TK_PIPE, "|"),
        TOKEN(TK_LT, "<"),
        TOKEN(TK_GT, ">"),
        TOKEN(TK_EQ_EQ, "=="),
        TOKEN(TK_BANG_EQ, "!="),
        TOKEN(TK_LT_EQ, "<="),
        TOKEN(TK_GT_EQ, ">=")
    };

    assert_tokens(lex, input, expected, "test");
    puts("operators\t==> passed");
}

void test_delimiters(Lexer lex) {
    char input[] = "() {} [] , -> : ; = in \n";

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
        TOKEN(TK_NL, "")
    };

    assert_tokens(lex, input, expected, "test");
    puts("delimiters\t==> passed");
}

void test_escaping(Lexer lex) {
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

    assert_tokens(lex, input, expected, "test");
    puts("escaping\t==> passed");
}

void test_errors(Lexer lex) {
    char input[] = "? 'inv\\z' 1e+ "
        "'\\777' '\\17' '\\888' "
        "'\\x1' '\\x1g' `unter";

    Token expected[] = {
        ERROR("?", "unrecognize syntax"),
        ERROR("z'", "invalid escape sequence"),
        ERROR("1e+", "malformed scientific notation"),
        ERROR("777'", "invalid escape sequence"),
        ERROR("17'", "invalid escape sequence"),
        ERROR("888'", "invalid escape sequence"),
        ERROR("1'", "invalid escape sequence"),
        ERROR("1g'", "invalid escape sequence"),
        ERROR("`unter", "unterminated string")
    };

    assert_tokens(lex, input, expected, "test");
    puts("errors\t\t==> passed");
}

/* test the inner functionality of the lexer */
void test_lexer() {
    Lexer lex = lexer_new("", "", R_FIRS);
    
    test_literals(lex);
    test_keywords(lex);
    test_identifiers(lex);
    test_delimiters(lex);
    test_escaping(lex);
    test_errors(lex);
}

/***************************************** *****************/

/* test the layers which uses/used by the lexer
   (e.g. scanfile, cons_tokens, list iteration ...) */
void test_file_lexing() {
    Token tok_expected[] = {
        NEW_TOKEN(TK_NL, "", 2), 
        NEW_TOKEN(TK_NL, "", 24),
        NEW_TOKEN(TK_NL, "", 25),
        NEW_TOKEN(TK_FN, "fn", 26),
        NEW_TOKEN(TK_IDENT, "derivative", 26),
        NEW_TOKEN(TK_LPAREN, "(", 26),
        NEW_TOKEN(TK_IDENT, "f", 26),
        NEW_TOKEN(TK_RPAREN, ")", 26),
        NEW_TOKEN(TK_NL, "", 26),
        NEW_TOKEN(TK_LET, "let", 27),
        NEW_TOKEN(TK_IDENT, "delta", 27),
        NEW_TOKEN(TK_EQ, "=", 27),
        NEW_TOKEN(TK_FLOAT, "1e-4", 27),
        NEW_TOKEN(TK_NL, "", 27),
        NEW_TOKEN(TK_RETURN, "return", 28),
        NEW_TOKEN(TK_FN, "fn", 28),
        NEW_TOKEN(TK_LPAREN, "(", 28),
        NEW_TOKEN(TK_IDENT, "n", 28),
        NEW_TOKEN(TK_RPAREN, ")", 28),
        NEW_TOKEN(TK_NL, "", 28),
        NEW_TOKEN(TK_LPAREN, "(", 29),
        NEW_TOKEN(TK_IDENT, "f", 29),
        NEW_TOKEN(TK_LPAREN, "(", 29),
        NEW_TOKEN(TK_IDENT, "n", 29),
        NEW_TOKEN(TK_PLUS, "+", 29),
        NEW_TOKEN(TK_IDENT, "delta", 29),
        NEW_TOKEN(TK_RPAREN, ")", 29),
        NEW_TOKEN(TK_MINUS, "-", 29),
        NEW_TOKEN(TK_IDENT, "f", 29),
        NEW_TOKEN(TK_LPAREN, "(", 29),
        NEW_TOKEN(TK_IDENT, "n", 29),
        NEW_TOKEN(TK_RPAREN, ")", 29),
        NEW_TOKEN(TK_RPAREN, ")", 29),
        NEW_TOKEN(TK_SLASH, "/", 29),
        NEW_TOKEN(TK_IDENT,"delta", 29),
        NEW_TOKEN(TK_NL, "", 29),
        NEW_TOKEN(TK_END, "end", 30),
        NEW_TOKEN(TK_NL, "", 30),
        NEW_TOKEN(TK_END, "end", 31),
        NEW_TOKEN(TK_NL, "", 31),
        NEW_TOKEN(TK_NL, "", 32),
        NEW_TOKEN(TK_EOF, "\0", 33)
    };

    Token err_expected[] = {
        NEW_ERROR("?", "unrecognize syntax", 33),
        NEW_ERROR("`unterminate", "unterminated string", 33) 
    };

    /* 
       NOTE: 
       run the test from the main directory,
       not from the /bin directory.
    */
    
    char *path = "tests/test_lexer.rav";
    char *buf = scan_file(path);
    assert(buf != NULL);
    
    Lexer lex = lexer_new(buf, "test", R_SECN);
    assert(lex != NULL);
    
    List tokens = cons_tokens(lex);

    /* there are lexing errors in the test file */
    assert(lexer_been_error(lex));

    /* assert non-error tokens */
    Token token = List_iter(tokens);
    for (int i = 0; token != NULL; i++) {
#ifdef PRINT_TOKENS
        print_token(token);
        print_token(tok_expected[i]);
        puts("");
#endif
        assert_token(token, tok_expected[i]);
        token = List_iter(tokens);
    }

    /* assert error tokens */
    List errors = lexer_errors(lex);
    token = List_iter(errors);
    for (int i = 0; token != NULL; i++) {
#ifdef PRINT_TOKENS
        print_token(token);
        print_token(err_expected[i]);
        puts("");
#endif
        assert_token(token, err_expected[i]);
        token = List_iter(errors);
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
