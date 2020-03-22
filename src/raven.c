
#include <stdio.h>

#include "lexer.h"

static void print_tokens(Lexer *lexer) {
    for (;;) {
        Token token = next_token(lexer);

        if (token.type == TOKEN_EOF) break;

        printf("%.*s\t", token.length, token.lexeme);
        printf("(%d)\n", token.type);
    }
}

int main(void) {
    char buf[128];
    Lexer lexer;

    for (;;) {
        fputs(">> ", stdout);
        if (!fgets(buf, 128, stdin) || feof(stdin))
            break;

        init_lexer(&lexer, buf);
        print_tokens(&lexer);
    }
}
