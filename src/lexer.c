#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "lexer.h"

void init_lexer(Lexer *lexer, const char *source) {
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

static inline bool at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

static inline char advance(Lexer *lexer) {
    return *lexer->current++;
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

static Token new_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    token.line = lexer->line;
    token.lexeme = lexer->start;
    token.length = lexer->current - lexer->start;
    
    return token;
}

static Token error_token(Lexer *lexer, const char *message) {
    fprintf(stderr, "[line %d] SyntaxError at '%.1s': %s\n",
            lexer->line, lexer->start, message);
    
    Token token;
    token.type = TOKEN_ERROR;
    token.line = lexer->line;
    token.lexeme = NULL;
    token.length = 0;

    return token;
}

static Token string(Lexer *lexer) {
    while (!at_end(lexer) && peek(lexer) != '\'' && peek(lexer) != '\n') {
        advance(lexer);
    }

    if (at_end(lexer) || peek(lexer) == '\n') {
        return error_token(lexer, "Unterminated string");
    }

    advance(lexer); // closing quote
    return new_token(lexer, TOKEN_STRING);
}

static TokenType match_keyword(Lexer *lexer, int start, int length,
                               const char *rest, TokenType type) {
    if (lexer->current - lexer->start == start + length &&
        memcmp(lexer->start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Lexer *lexer) {
    switch (lexer->start[0]) {
    case 'a': return match_keyword(lexer, 1, 2, "nd", TOKEN_AND);
    case 'b': return match_keyword(lexer, 1, 4, "reak", TOKEN_BREAK);
    case 'c': return match_keyword(lexer, 1, 3, "ond", TOKEN_COND);
    case 'd': return match_keyword(lexer, 1, 1, "o", TOKEN_DO);
    case 'l': return match_keyword(lexer, 1, 2, "et", TOKEN_LET);
    case 'm': return match_keyword(lexer, 1, 4, "atch", TOKEN_MATCH);
    case 'o': return match_keyword(lexer, 1, 1, "r", TOKEN_OR);
    case 'r': return match_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
    case 'w': return match_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
        
    case 'e':
        if (lexer->current - lexer->start == 1) break;
        
        switch (lexer->start[1]) {
        case 'n': return match_keyword(lexer, 2, 1, "d", TOKEN_END);
        case 'l': return match_keyword(lexer, 2, 2, "se", TOKEN_ELSE);
        }
        break;

    case 'f':
        if (lexer->current - lexer->start == 1) break;
        
        switch (lexer->start[1]) {
        case 'a': return match_keyword(lexer, 2, 3, "lse", TOKEN_FALSE);
        case 'n': return match_keyword(lexer, 2, 0, "", TOKEN_FN);
        case 'o': return match_keyword(lexer, 2, 1, "r", TOKEN_FOR);
        }
        break;


    case 'n':
        if (lexer->current - lexer->start == 1) break;

        switch (lexer->start[1]) {
        case 'i': return match_keyword(lexer, 2, 1, "l", TOKEN_NIL);
        case 'o': return match_keyword(lexer, 2, 1, "t", TOKEN_NOT);
        }
        break;

    case 'i':
        if (lexer->current - lexer->start == 1) break;

        switch (lexer->start[1]) {
        case 'f': return match_keyword(lexer, 2, 0, "", TOKEN_IF);
        case 'n': return match_keyword(lexer, 2, 0, "", TOKEN_IN);
        }
        break;
 

    case 't':
        if (lexer->current - lexer->start == 1) break;

        switch (lexer->start[1]) {
        case 'r': return match_keyword(lexer, 2, 2, "ue", TOKEN_TRUE);
        case 'y': return match_keyword(lexer, 2, 2, "pe", TOKEN_TYPE);
        }
        break;
    }

    return TOKEN_IDENTIFIER;
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
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;

    if (at_end(lexer)) return new_token(lexer, TOKEN_EOF);

    char c = advance(lexer);
    switch (c) {
        // One-Character Tokens
    case '+': return new_token(lexer, TOKEN_PLUS);
    case '*': return new_token(lexer, TOKEN_STAR);
    case '/': return new_token(lexer, TOKEN_SLASH);
    case '%': return new_token(lexer, TOKEN_PERCENT);
    case '.': return new_token(lexer, TOKEN_DOT);
    case '@': return new_token(lexer, TOKEN_AT);
    case '|': return new_token(lexer, TOKEN_PIPE);
    case ',': return new_token(lexer, TOKEN_COMMA);
    case '(': return new_token(lexer, TOKEN_LEFT_PAREN);
    case ')': return new_token(lexer, TOKEN_RIGHT_PAREN);
    case '[': return new_token(lexer, TOKEN_LEFT_BRACKET);
    case ']': return new_token(lexer, TOKEN_RIGHT_BRACKET);
    case '{': return new_token(lexer, TOKEN_LEFT_BRACE);
    case '}': return new_token(lexer, TOKEN_RIGHT_BRACE);
    case ';': return new_token(lexer, TOKEN_SEMICOLON);

        // Possible Two-Charachter Tokens
    case '-':
        if (match(lexer, '>')) return new_token(lexer, TOKEN_HYPHEN_LT);
        return new_token(lexer, TOKEN_MINUS);
        
    case '=':
        if (match(lexer, '=')) return new_token(lexer, TOKEN_EQUAL_EQUAL);
        return new_token(lexer, TOKEN_EQUAL);

    case '<':
        if (match(lexer, '=')) return new_token(lexer, TOKEN_LESS_EQUAL);
        return new_token(lexer, TOKEN_LESS);

    case '>':
        if (match(lexer, '=')) return new_token(lexer, TOKEN_GREATER_EQUAL);
        return new_token(lexer, TOKEN_GREATER);
        
    case ':':
        if (match(lexer, ':')) return new_token(lexer, TOKEN_COLON_COLON);
        return new_token(lexer, TOKEN_COLON);

    case '!':
        if (match(lexer, '=')) return new_token(lexer, TOKEN_BANG_EQUAL);
        break;

        // Identifiers, Literals
    case '\'': return string(lexer);

    default:
        if (isalpha(c) || c == '_') return identifier(lexer);
        if (isdigit(c)) return number(lexer);
    }

    return error_token(lexer, "Unexpected symbol");
}
