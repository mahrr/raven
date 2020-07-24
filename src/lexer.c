#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "lexer.h"

void init_lexer(Lexer *lexer, const char *source, const char *file) {
    lexer->file = file;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

static inline bool at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

static inline char advance(Lexer *lexer) {
    return at_end(lexer) ? '\0' : *lexer->current++;
}

static inline char peek(Lexer *lexer) {
    return *lexer->current;
}

static inline char peek_next(Lexer *lexer) {
    if (at_end(lexer)) return '\0';
    return lexer->current[1];
}

static inline bool match(Lexer *lexer, char expected) {
    if (!at_end(lexer) && peek(lexer) == expected) {
        lexer->current++;
        return true;
    }

    return false;
}

static inline Token new_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    token.line = lexer->line;
    token.lexeme = lexer->start;
    token.length = lexer->current - lexer->start;
    
    return token;
}

static Token error_token(Lexer *lexer, const char *message) {
    fprintf(stderr, "[%s: %d] SyntaxError at '%.1s': %s\n",
            lexer->file, lexer->line, lexer->start, message);
    
    Token token;
    token.type = TOKEN_ERROR;
    token.line = lexer->line;
    token.lexeme = NULL;
    token.length = 0;

    return token;
}

static Token string(Lexer *lexer) {
    while (!at_end(lexer) &&
           peek(lexer) != '\'' &&
           peek(lexer) != '\n') {
        advance(lexer);
    }

    if (at_end(lexer) || peek(lexer) == '\n') {
        return error_token(lexer, "Unterminated string");
    }

    advance(lexer); // closing quote
    return new_token(lexer, TOKEN_STRING);
}

static inline TokenType is_keyword(Lexer *lexer, int start, int length,
                                   const char *rest, TokenType type) {
    if (lexer->current - lexer->start == start + length &&
        memcmp(lexer->start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Lexer *lexer) {
#define Is_Keyword(start, rest, type)                       \
    is_keyword(lexer, start, sizeof(rest) - 1, rest, type)

    switch (lexer->start[0]) {
    case 'b': return Is_Keyword(1, "reak",  TOKEN_BREAK);
    case 'd': return Is_Keyword(1, "o",     TOKEN_DO);
    case 'l': return Is_Keyword(1, "et",    TOKEN_LET);
    case 'm': return Is_Keyword(1, "atch",  TOKEN_MATCH);
    case 'o': return Is_Keyword(1, "r",     TOKEN_OR);
    case 'r': return Is_Keyword(1, "eturn", TOKEN_RETURN);
    case 'w': return Is_Keyword(1, "hile",  TOKEN_WHILE);

    case 'a':
        if (lexer->current - lexer->start < 3) break;

        switch (lexer->start[1]) {
        case 'n': return Is_Keyword(2, "d",    TOKEN_AND);
        case 's': return Is_Keyword(2, "sert", TOKEN_ASSERT);
        }
        break;

    case 'c':
        if (lexer->current - lexer->start < 4) break;
        if (lexer->start[1] != 'o') break;
        if (lexer->start[2] != 'n') break;

        switch (lexer->start[3]) {
        case 'd': return Is_Keyword(4, "",     TOKEN_COND);
        case 't': return Is_Keyword(4, "inue", TOKEN_CONTINUE);
        }
        break;
        
    case 'e':
        if (lexer->current - lexer->start < 3) break;
        
        switch (lexer->start[1]) {
        case 'n': return Is_Keyword(2, "d",  TOKEN_END);
        case 'l': return Is_Keyword(2, "se", TOKEN_ELSE);
        }
        break;

    case 'f':
        if (lexer->current - lexer->start < 2) break;
        
        switch (lexer->start[1]) {
        case 'a': return Is_Keyword(2, "lse", TOKEN_FALSE);
        case 'n': return Is_Keyword(2, "",    TOKEN_FN);
        case 'o': return Is_Keyword(2, "r",   TOKEN_FOR);
        }
        break;


    case 'n':
        if (lexer->current - lexer->start != 3) break;

        switch (lexer->start[1]) {
        case 'i': return Is_Keyword(2, "l", TOKEN_NIL);
        case 'o': return Is_Keyword(2, "t", TOKEN_NOT);
        }
        break;

    case 'i':
        if (lexer->current - lexer->start != 2) break;

        switch (lexer->start[1]) {
        case 'f': return TOKEN_IF;
        case 'n': return TOKEN_IN;
        }
        break;
 

    case 't':
        if (lexer->current - lexer->start < 4) break;

        switch (lexer->start[1]) {
        case 'r': return Is_Keyword(2, "ue", TOKEN_TRUE);
        case 'y': return Is_Keyword(2, "pe", TOKEN_TYPE);
        }
        break;
    }

    return TOKEN_IDENTIFIER;

#undef Is_Keyword
}

static Token identifier(Lexer *lexer) {
    while (!at_end(lexer) &&
           (isalnum(peek(lexer)) || peek(lexer) == '_')) {
        advance(lexer);
    }

    TokenType type = identifier_type(lexer);
    return new_token(lexer, type);
}

static Token number(Lexer *lexer) {
    if (lexer->current[-1] == '0') {
        switch (peek(lexer)) {
        case 'x':
        case 'X':
            advance(lexer); // consume 'x'
            while (isxdigit(peek(lexer))) {
                advance(lexer);
            }
            return new_token(lexer, TOKEN_NUMBER);

        case 'o':
        case 'O':
            advance(lexer); // consume 'b'
            while (isdigit(peek(lexer)) && peek(lexer) < '8') {
                advance(lexer);
            }
            return new_token(lexer, TOKEN_NUMBER);
            
        case 'b':
        case 'B':
            advance(lexer); // consume 'b'
            while (peek(lexer) == '0' || peek(lexer) == '1') {
                advance(lexer);
            }
            return new_token(lexer, TOKEN_NUMBER);
        }
    }

    while (isdigit(peek(lexer))) advance(lexer);

    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer); // consume dot
        while (isdigit(peek(lexer))) advance(lexer);
    }

    return new_token(lexer, TOKEN_NUMBER);
}

static void skip_whitespace(Lexer *lexer) {
    for (;;) {
        switch (peek(lexer)) {
        case ' ':
        case '\t':
        case '\r':
            advance(lexer);
            break;

        case '\n':
            lexer->line++;
            advance(lexer);
            break;

        case '#':
            while (!at_end(lexer) && peek(lexer) != '\n') advance(lexer);
            break;

        default:
            return;
        }
    }
}

Token next_token(Lexer *lexer) {
#define New_Token(type) (new_token(lexer, type))
    
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;

    if (at_end(lexer)) return New_Token(TOKEN_EOF);

    char c = advance(lexer);
    switch (c) {
        // One-Character Tokens
    case '+': return New_Token(TOKEN_PLUS);
    case '*': return New_Token(TOKEN_STAR);
    case '/': return New_Token(TOKEN_SLASH);
    case '%': return New_Token(TOKEN_PERCENT);
    case '.': return New_Token(TOKEN_DOT);
    case '@': return New_Token(TOKEN_AT);
    case '|': return New_Token(TOKEN_PIPE);
    case ',': return New_Token(TOKEN_COMMA);
    case '(': return New_Token(TOKEN_LEFT_PAREN);
    case ')': return New_Token(TOKEN_RIGHT_PAREN);
    case '[': return New_Token(TOKEN_LEFT_BRACKET);
    case ']': return New_Token(TOKEN_RIGHT_BRACKET);
    case '{': return New_Token(TOKEN_LEFT_BRACE);
    case '}': return New_Token(TOKEN_RIGHT_BRACE);
    case ';': return New_Token(TOKEN_SEMICOLON);

        // Possible Two-Charachter Tokens
    case '-':
        if (match(lexer, '>')) return New_Token(TOKEN_ARROW);
        return New_Token(TOKEN_MINUS);
        
    case '=':
        if (match(lexer, '=')) return New_Token(TOKEN_EQUAL_EQUAL);
        return New_Token(TOKEN_EQUAL);

    case '<':
        if (match(lexer, '=')) return New_Token(TOKEN_LESS_EQUAL);
        return New_Token(TOKEN_LESS);

    case '>':
        if (match(lexer, '=')) return New_Token(TOKEN_GREATER_EQUAL);
        return New_Token(TOKEN_GREATER);
        
    case ':':
        if (match(lexer, ':')) return New_Token(TOKEN_COLON_COLON);
        return New_Token(TOKEN_COLON);

    case '!':
        if (match(lexer, '=')) return New_Token(TOKEN_BANG_EQUAL);
        break;

        // Identifiers, Literals
    case '\'': return string(lexer);

    default:
        if (isalpha(c) || c == '_') return identifier(lexer);
        if (isdigit(c)) return number(lexer);
    }

    return error_token(lexer, "Unexpected symbol");
#undef New_Token    
}
