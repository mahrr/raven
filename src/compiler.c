#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "lexer.h"
#include "vm.h"

/*
 * A Pratt parser is used for expression parsing. A parsing rule
 * is defined with a ParseRule struct. The parser uses a parsing
 * rule table to know the precedence, prefix parsing function and
 * parsing infix function for each token type.
*/

// Parser state
typedef struct {
    Lexer *lexer;     // The input, token stream
    VM *vm;           // The output chunk, bytecode stream
    
    Token current;    // Current consumed token
    Token previous;   // Previously consumed token
    
    bool had_error;   // Error flag to stop bytecode execution later
    bool panic_mode;  // If set, any parsing error will be ignored
} Parser;

// Expressions precedence, from low to high
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_CONS,        // ::
    PREC_CONCAT,      // @
    PREC_TERM,        // + -
    PREC_FACTOR,      // * / %
    PREC_UNARY,       // not -
    PREC_CALL,        // ()
    PREC_HIGHEST,     // Group [] .
} Precedence;

typedef void (*ParseFn)(Parser *);

// Parser rule for a token type
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

/** Error Reporting **/

static void error(Parser *parser, Token *where, const char *message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;

    fprintf(stderr, "[line %d] Error", where->line);

    if (where->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (where->type != TOKEN_ERROR) {
        fprintf(stderr, " at '%.*s'", where->length, where->lexeme);
    }

    fprintf(stderr, ": %s\n", message);
    parser->had_error = true;
}

static inline void error_previous(Parser *parser, const char *message) {
    error(parser, &parser->previous, message);
}

static inline void error_current(Parser *parser, const char *message) {
    error(parser, &parser->current, message);
}

/** Parser State **/

static void advance(Parser *parser) {
    parser->previous = parser->current;
    
    for (;;) {
        parser->current = next_token(parser->lexer);

        if (parser->current.type != TOKEN_ERROR) break;
        error_current(parser, parser->current.lexeme);
    }
}

static void consume(Parser *parser, TokenType type, const char *msg) {
    if (parser->current.type == type) {
        advance(parser);
    } else {
        error_current(parser, msg);
    }
}

/** Emitting **/

static inline void emit_byte(Parser *parser, uint8_t byte) {
    write_byte(parser->vm->chunk, byte, parser->previous.line);
}

static inline void emit_bytes(Parser *parser, uint8_t x, uint8_t y) {
    emit_byte(parser, x);
    emit_byte(parser, y);
}

static uint8_t make_constant(Parser *parser, Value value) {
    int constant_index = write_constant(parser->vm->chunk, value);

    if (constant_index > UINT8_MAX) {
        error_current(parser, "Too many constants in one chunk");
        return 0;
    }

    return (uint8_t)constant_index;
}

static inline void emit_constant(Parser *parser, Value value) {
    emit_bytes(parser, OP_LOAD_CONST, make_constant(parser, value));
}

/** Parsing **/

static void expression(Parser *);
static void parse(Parser *, Precedence);
static ParseRule *token_rule(TokenType);

static void binary(Parser *parser) {
    TokenType operator = parser->previous.type;

    ParseRule *rule = token_rule(operator);
    parse(parser, (Precedence)(rule->precedence + 1));

    switch (operator) {
    case TOKEN_PLUS:    emit_byte(parser, OP_ADD); break;
    case TOKEN_MINUS:   emit_byte(parser, OP_SUB); break;
    case TOKEN_STAR:    emit_byte(parser, OP_MUL); break;
    case TOKEN_SLASH:   emit_byte(parser, OP_DIV); break;
    case TOKEN_PERCENT: emit_byte(parser, OP_MOD); break;
    default:
        assert(0);
    }
}

static void grouping(Parser *parser) {
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN,
            "Expect closing ')' after group expression");
}

static void number(Parser *parser) {
    Value value = NUM_VALUE(strtod(parser->previous.lexeme, NULL));
    emit_constant(parser, value);
}

static void unary(Parser *parser) {
    TokenType operator = parser->previous.type;
    parse(parser, PREC_UNARY);

    switch (operator) {
    case TOKEN_MINUS: emit_byte(parser, OP_NEG); break;
    case TOKEN_NOT:   emit_byte(parser, OP_NOT); break;
    default:
        assert(0);
    }
}

// Parsing rule table
static ParseRule rules[] = {
    { NULL,     NULL,   PREC_NONE },      // TOKEN_BREAK
    { NULL,     NULL,   PREC_NONE },      // TOKEN_COND
    { NULL,     NULL,   PREC_NONE },      // TOKEN_ELSE
    { NULL,     NULL,   PREC_NONE },      // TOKEN_FALSE
    { NULL,     NULL,   PREC_NONE },      // TOKEN_FN
    { NULL,     NULL,   PREC_NONE },      // TOKEN_FOR
    { NULL,     NULL,   PREC_NONE },      // TOKEN_IF
    { NULL,     NULL,   PREC_NONE },      // TOKEN_IN
    { NULL,     NULL,   PREC_NONE },      // TOKEN_LET
    { NULL,     NULL,   PREC_NONE },      // TOKEN_MATCH
    { NULL,     NULL,   PREC_NONE },      // TOKEN_NIL
    { NULL,     NULL,   PREC_NONE },      // TOKEN_RETURN
    { NULL,     NULL,   PREC_NONE },      // TOKEN_WHILE
    { NULL,     NULL,   PREC_NONE },      // TOKEN_TRUE
    { NULL,     NULL,   PREC_NONE },      // TOKEN_TYPE
    { NULL,     binary, PREC_TERM },      // TOKEN_PLUS
    { unary,    binary, PREC_TERM },      // TOKEN_MINUS
    { NULL,     binary, PREC_FACTOR },    // TOKEN_STAR
    { NULL,     binary, PREC_FACTOR },    // TOKEN_SLASH
    { NULL,     binary, PREC_FACTOR },    // TOKEN_PERCENT
    { NULL,     NULL,   PREC_NONE },      // TOKEN_DOT
    { unary,    NULL,   PREC_NONE },      // TOKEN_NOT
    { NULL,     NULL,   PREC_NONE },      // TOKEN_AND
    { NULL,     NULL,   PREC_NONE },      // TOKEN_OR
    { NULL,     NULL,   PREC_NONE },      // TOKEN_AT
    { NULL,     NULL,   PREC_NONE },      // TOKEN_COLON_COLON
    { NULL,     NULL,   PREC_NONE },      // TOKEN_LT
    { NULL,     NULL,   PREC_NONE },      // TOKEN_LT_EQUAL
    { NULL,     NULL,   PREC_NONE },      // TOKEN_GT
    { NULL,     NULL,   PREC_NONE },      // TOKEN_GT_EQUAL
    { NULL,     NULL,   PREC_NONE },      // TOKEN_EQUAL
    { NULL,     NULL,   PREC_NONE },      // TOKEN_EQUAL_EQUAL
    { NULL,     NULL,   PREC_NONE },      // TOKEN_BANG_EQUAL
    { NULL,     NULL,   PREC_NONE },      // TOKEN_DO
    { NULL,     NULL,   PREC_NONE },      // TOKEN_END
    { NULL,     NULL,   PREC_NONE },      // TOKEN_PIPE
    { NULL,     NULL,   PREC_NONE },      // TOKEN_HYPHEN_LT
    { NULL,     NULL,   PREC_NONE },      // TOKEN_COMMA
    { NULL,     NULL,   PREC_NONE },      // TOKEN_SEMICOLON
    { NULL,     NULL,   PREC_NONE },      // TOKEN_COLON
    { grouping, NULL,   PREC_NONE },      // TOKEN_LEFT_PAREN
    { NULL,     NULL,   PREC_NONE },      // TOKEN_RIGHT_PAREN
    { NULL,     NULL,   PREC_NONE },      // TOKEN_LEFT_BRACE
    { NULL,     NULL,   PREC_NONE },      // TOKEN_RIGHT_BRACE
    { NULL,     NULL,   PREC_NONE },      // TOKEN_LEFT_BRACKET
    { NULL,     NULL,   PREC_NONE },      // TOKEN_RIGHT_BRACKET
    { NULL,     NULL,   PREC_NONE },      // TOKEN_IDENTIFIER
    { number,   NULL,   PREC_NONE },      // TOKEN_NUMBER
    { NULL,     NULL,   PREC_NONE },      // TOKEN_STRING
    { NULL,     NULL,   PREC_NONE },      // TOKEN_ERROR
    { NULL,     NULL,   PREC_NONE },      // TOKEN_EOF
};

// Return the parsing rule of a given token type.
static inline ParseRule *token_rule(TokenType type) {
    return &rules[type];
}

static void parse(Parser *parser, Precedence precedence) {
    advance(parser);

    ParseFn prefix = token_rule(parser->previous.type)->prefix;
    if (prefix == NULL) {
        error_previous(parser, "Unexpected token, expect expression");
        return;
    }

    prefix(parser);
    while (precedence <= token_rule(parser->current.type)->precedence) {
        advance(parser);
        ParseFn infix = token_rule(parser->previous.type)->infix;
        infix(parser);
    }
}

static inline void expression(Parser *parser) {
    parse(parser, PREC_ASSIGNMENT);
}

bool compile(VM *vm, const char *source) {
    Lexer lexer;
    init_lexer(&lexer, source);
    
    Parser parser;
    parser.lexer = &lexer;
    parser.vm = vm;
    parser.had_error = false;
    parser.panic_mode = false;

    advance(&parser);
    expression(&parser);
    consume(&parser, TOKEN_EOF, "Expect end of expression");

    emit_byte(&parser, OP_RETURN);
    return !parser.had_error;
}
