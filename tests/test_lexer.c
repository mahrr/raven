/*
 *
 * (test_lexer.c | 10 Dec 18 | Amr Anwar, Kareem Hamdy)
 *
 * test for lexer.c code 
*/

#include <assert.h>
#include <stdio.h>

#include "../src/alloc.h"
#include "../src/error.h"
#include "../src/lexer.h"
#include "../src/list.h"

token_list *cons_tokens_test(token *toks[]) {
    list *tokens = NULL;
    list *error_tokens = NULL;
    bool been_error = false;
    token *tok = toks[0];
    
    for (int i = 1; tok->type != EOF_TOK; i++) {
        if (is_errtok(tok)) {
            been_error = true;
            error_tokens = append_list(error_tokens, tok);
        }
        tokens = append_list(tokens, tok);
        tok = toks[i];
    }
    
    token_list *tl = make(tl, R_FIRS);
    tl->tokens = tokens;
    tl->been_error = been_error;
    tl->error_tokens = error_tokens;
    return tl;
}

token *create_tok(token_type type, char *lexeme, int line) {
    token *tok = make(tok, R_FIRS);
    tok->lexeme = lexeme;
    tok->type = type;
    tok->line = line;
    return tok;
}

void compare_two_tokens(token *input_tk, token *test_tk) {
    assert(input_tk->type == test_tk->type);
    if (input_tk->lexeme == NULL || test_tk->lexeme == NULL)
        assert(input_tk->lexeme == NULL && test_tk->lexeme == NULL);
    else
        assert(strcmp(input_tk->lexeme, test_tk->lexeme) == 0);
    assert(input_tk->line == test_tk->line);
}

void compare_two_lists(list *input, list *test) {
    int input_len = 0, test_len = 0;

    while (test != NULL || input != NULL) {
        token *curr_input_tk = input->obj;
        token *curr_test_tk = test->obj;
        
        compare_two_tokens(curr_input_tk, curr_test_tk);
        input = input->link;
        test = test->link;
        
        if (test != NULL) test_len++;
        if (input != NULL) input_len++;
        
        assert(input_len == test_len);
    }
}

void compare_two_token_lists(token_list *input, token_list *test) {
    compare_two_lists(input->tokens, test->tokens);
    compare_two_lists(input->error_tokens, test->error_tokens);
    assert(input->been_error == test->been_error);
}

void init_test(char *str_input, token *toks[]) {
    lexer *lex = make(lex, R_FIRS);

    printf("\n--------TEST CASE--------\n");
    printf("input: '%s'\n", str_input);
    // test date
    token_list *test_toks = cons_tokens_test(toks);
    // input date
    init_lexer(lex, str_input, "stdin");
    token_list *input_tokens = cons_tokens(lex);
    // comparing
    compare_two_token_lists(input_tokens, test_toks);
    printf("------ passed----------\n");
}

void test_keywords() {
    
    token *toks[] = {
        create_tok(FN, NULL, 1),
        create_tok(TYPE, NULL, 1),
        create_tok(RETURN, NULL, 1),
        create_tok(LET, NULL, 1),
        create_tok(FIN, NULL, 1),
        create_tok(USE, NULL, 1),
        create_tok(DO, NULL, 1),
        create_tok(END, NULL, 1),
        create_tok(IF, NULL, 1),
        create_tok(THEN, NULL, 1),
        create_tok(ELIF, NULL, 1),
        create_tok(ELSE, NULL, 1),
        create_tok(FOR, NULL, 1),
        create_tok(WHILE, NULL, 1),
        create_tok(CONTINUE, NULL, 1),
        create_tok(BREAK, NULL, 1),
        create_tok(RAISE, NULL, 1),
        create_tok(HANDLE, NULL, 1),
        create_tok(MATCH, NULL, 1),
        create_tok(CASE, NULL, 1),
        create_tok(EOF_TOK, "", 1)
    };
    
    char *str =
        " fn type return let fin use do end if then elif \
    else for while continue break raise handle match case";

    init_test(str, toks);
}

void test_literals_types() {
    char *str =
        "1500 0.5 \"kareem\" !\"kareem\" \
     false true nil \n  int float str bool list";
    token *toks[] = {
        create_tok(INT, "1500", 1),
        create_tok(FLOAT, "0.5", 1),
        create_tok(STRING, "kareem", 1),
        create_tok(R_STRING, "!\"kareem\"", 1),
        create_tok(FALSE, NULL, 1),
        create_tok(TRUE, NULL, 1),
        create_tok(NIL, NULL, 1),
        create_tok(TK_NL, NULL, 1),
        /* Types */
        create_tok(INT_T, NULL, 2),
        create_tok(FLOAT_T, NULL, 2),
        create_tok(STR_T, NULL, 2),
        create_tok(BOOL_T, NULL, 2),
        create_tok(LIST_T, NULL, 2),
        create_tok(EOF_TOK, "", 1)
    };

    init_test(str, toks);
}

/* Operators Arithmetic Operators Ordering Operators Logic Operators Delimiters */
void test_symbol_tokens() {
    char *str = "= and or not. .. #@ : => >| +-* / % < > == != <= >= |&^ ~ >> << (){}[ ],";
    token *toks[] = {
        /* Operators */
        create_tok(EQUAL, NULL, 1),
        create_tok(AND, NULL, 1),
        create_tok(OR, NULL, 1),
        create_tok(NOT, NULL, 1),
        create_tok(DOT, NULL, 1),
        create_tok(DOT_DOT, NULL, 1),
        create_tok(HASH, NULL, 1),
        create_tok(AT, NULL, 1),
        create_tok(COLON, NULL, 1),
        create_tok(EQUAL_GREAT, NULL, 1),
        create_tok(GREAT_PIPE, NULL, 1),
        /* Arthimetik Operators */
        create_tok(PLUS, NULL, 1),
        create_tok(MINUS, NULL, 1),
        create_tok(ASTERISK, NULL, 1),
        create_tok(SLASH, NULL, 1),
        create_tok(PERCENT, NULL, 1),
        /* Ordering Operators */
        create_tok(LESS, NULL, 1),
        create_tok(GREAT, NULL, 1),
        create_tok(EQUAL_EQUAL, NULL, 1),
        create_tok(BANG_EQUAL, NULL, 1),
        create_tok(LESS_EQUAL, NULL, 1),
        create_tok(GREAT_EQUAL, NULL, 1),
        /* Logic Operators */
        create_tok(PIPE, NULL, 1),
        create_tok(AMPERSAND, NULL, 1),
        create_tok(CARET, NULL, 1),
        create_tok(TILDE, NULL, 1),
        create_tok(GREAT_GREAT, NULL, 1),
        create_tok(LESS_LESS, NULL, 1),
        /* Delimiters */
        create_tok(LPAREN, NULL, 1),
        create_tok(RPAREN, NULL, 1),
        create_tok(LBRACE, NULL, 1),
        create_tok(RBRACE, NULL, 1),
        create_tok(LBRACKET, NULL, 1),
        create_tok(RBRACKET, NULL, 1),
        create_tok(COMMA, NULL, 1),
        create_tok(EOF_TOK, "", 1)
    };

    init_test(str, toks);
}

void combination_test() {
    char *str = "let kareem = 1500\nkareem=kareem+15.5\nkareem=(15.5+1500)";
    
    token *toks[] = {
        /*frist line */
        create_tok(LET, NULL, 1),
        create_tok(IDENT, "kareem", 1),
        create_tok(EQUAL, NULL, 1),
        create_tok(INT, "1500", 1),
        create_tok(TK_NL, NULL, 1),
        /*sce line */
        create_tok(IDENT, "kareem", 2),
        create_tok(EQUAL, NULL, 2),
        create_tok(IDENT, "kareem", 2),
        create_tok(PLUS, NULL, 2),
        create_tok(FLOAT, "15.5", 2),
        create_tok(TK_NL, NULL, 2),
        /*third line */
        create_tok(IDENT, "kareem", 3),
        create_tok(EQUAL, NULL, 3),
        create_tok(LPAREN, NULL, 3),
        create_tok(FLOAT, "15.5", 3),
        create_tok(PLUS, NULL, 3),
        create_tok(INT, "1500", 3),
        create_tok(RPAREN, NULL, 3),
        create_tok(EOF_TOK, "", 3)

    };
    init_test(str, toks);

    /* test other shape of Integer */
    char *str2 = "let kareem = 0x65Eeac \ne=0o123456755\n i=(0b11011)";
    
    token *toks2[] = {
        /*frist line  test Hexa */
        create_tok(LET, NULL, 1),
        create_tok(IDENT, "kareem", 1),
        create_tok(EQUAL, NULL, 1),
        create_tok(INT, "0x65Eeac", 1),
        create_tok(TK_NL, NULL, 1),
        /*sce line test Octal*/
        create_tok(IDENT, "e", 2),
        create_tok(EQUAL, NULL, 2),
        create_tok(INT, "0o123456755", 2),
        create_tok(TK_NL, NULL, 2),
        /*third line test Binary*/
        create_tok(IDENT, "i", 3),
        create_tok(EQUAL, NULL, 3),
        create_tok(LPAREN, NULL, 3),
        create_tok(INT, "0b11011", 3),
        create_tok(RPAREN, NULL, 3),
        create_tok(EOF_TOK, "", 3)
    };
    
    init_test(str2, toks2);
}

int main(void) {
    test_keywords();
    test_literals_types();
    test_symbol_tokens();
    combination_test();
    return 0;
}
