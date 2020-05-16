#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "lexer.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#if defined(DEBUG_DUMP_CODE) || defined(DEBUG_TRACE_PARSING)
#include <stdarg.h>
#include "debug.h"
#endif

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

#ifdef DEBUG_TRACE_PARSING
    int level;        // Parser nesting level, for debugging
#endif
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

/** Parser Tracing **/

#ifdef DEBUG_TRACE_PARSING

static const char *precedence_string[] = {
    "None",
    "Assignment",
    "Or",
    "And",
    "Equality",
    "Comparison",
    "Cons",
    "Concat",
    "Term",
    "Factor",
    "Unary",
    "Call",
    "Highest",
};

static void debug_log(Parser *parser, const char *fmt, ...) {
    va_list arguments;
    va_start(arguments, fmt);

    for (int i = 0; i < parser->level; i++) printf("| ");
    vprintf(fmt, arguments);
    putchar('\n');

    va_end(arguments);
    parser->level += 1;
}

#define Debug_Logf(parser, fmt, ...)            \
    debug_log(parser, fmt, ##__VA_ARGS__)

#define Debug_Log(parser) debug_log(parser, __func__);
#define Debug_Exit(parser) (parser)->level -= 1

#else

#define Debug_Logf(parser, fmt, ...)
#define Debug_Log(parser)
#define Debug_Exit(parser)

#endif

/** Error Reporting **/

static void error(Parser *parser, Token *where, const char *message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;

    fprintf(stderr, "[%s: %d] SyntaxError",
            parser->lexer->file, where->line);

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

static inline void advance(Parser *parser) {
    parser->previous = parser->current;
    
    for (;;) {
        parser->current = next_token(parser->lexer);

        if (parser->current.type != TOKEN_ERROR) break;
    }
}

static inline void consume(Parser *parser, TokenType type,
                           const char *msg) {
    if (parser->current.type == type) {
        advance(parser);
    } else {
        error_current(parser, msg);
    }
}

static inline bool check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static inline bool match(Parser *parser, TokenType type) {
    if (!check(parser, type)) return false;

    advance(parser);
    return true;
}

/** Emitting **/

static inline void emit_byte(Parser *parser, uint8_t byte) {
    write_byte(parser->vm->chunk, byte, parser->previous.line);
}

static inline void emit_bytes(Parser *parser, uint8_t x, uint8_t y) {
    emit_byte(parser, x);
    emit_byte(parser, y);
}

static inline int emit_jump(Parser *parser, uint8_t instruction) {
    emit_byte(parser, instruction);
    emit_bytes(parser, 0xff, 0xff);
    return parser->vm->chunk->count - 2;
}

static inline void patch_jump(Parser *parser, int from) {
    // -2 because of the jmp instruction 2-bytes immediate argument
    int offset = parser->vm->chunk->count - from - 2;

    if (offset > UINT16_MAX) {
        error_current(parser, "Jump offset exceeds the allowed limit");
    }

    parser->vm->chunk->opcodes[from] = (offset >> 8) & 0xff;
    parser->vm->chunk->opcodes[from + 1] = offset & 0xff;
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

static void parse_precedence(Parser*, Precedence);
static inline void expression(Parser*);
static inline ParseRule *token_rule(TokenType);

static void binary(Parser *parser) {
    Debug_Log(parser);
    
    TokenType operator = parser->previous.type;

    ParseRule *rule = token_rule(operator);
    parse_precedence(parser, (Precedence)(rule->precedence + 1));

    switch (operator) {
    case TOKEN_PLUS:          emit_byte(parser, OP_ADD); break;
    case TOKEN_MINUS:         emit_byte(parser, OP_SUB); break;
    case TOKEN_STAR:          emit_byte(parser, OP_MUL); break;
    case TOKEN_SLASH:         emit_byte(parser, OP_DIV); break;
    case TOKEN_PERCENT:       emit_byte(parser, OP_MOD); break;
    case TOKEN_LESS:          emit_byte(parser, OP_LT);  break;
    case TOKEN_LESS_EQUAL:    emit_byte(parser, OP_LTQ); break;
    case TOKEN_GREATER:       emit_byte(parser, OP_GT);  break;
    case TOKEN_GREATER_EQUAL: emit_byte(parser, OP_GTQ); break;
    case TOKEN_EQUAL_EQUAL:   emit_byte(parser, OP_EQ);  break;
    case TOKEN_BANG_EQUAL:    emit_byte(parser, OP_NEQ); break;
    default:
        assert(!"invalid token type");
    }

    Debug_Exit(parser);
}

static void and_(Parser *parser) {
    Debug_Log(parser);

    int jump = emit_jump(parser, OP_JMP_FALSE);

    emit_byte(parser, OP_POP);
    parse_precedence(parser, PREC_AND + 1);

    patch_jump(parser, jump);

    Debug_Exit(parser);
}

static void or_(Parser *parser) {
    Debug_Log(parser);

    // first operand is falsy
    int false_jump = emit_jump(parser, OP_JMP_FALSE);

    // first operand is not falsy
    int true_jump = emit_jump(parser, OP_JMP);

    patch_jump(parser, false_jump);

    emit_byte(parser, OP_POP);
    parse_precedence(parser, PREC_OR + 1);

    patch_jump(parser, true_jump);

    Debug_Exit(parser);
}

static void grouping(Parser *parser) {
    Debug_Log(parser);
    
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN,
            "Expect closing ')' after group expression");

    Debug_Exit(parser);
}

static void number(Parser *parser) {
    Debug_Log(parser);
    
    Value value = Num_Value(strtod(parser->previous.lexeme, NULL));
    emit_constant(parser, value);

    Debug_Exit(parser);
}

static void string(Parser *parser) {
    Debug_Log(parser);

    // +1 and -2 for the literal string quotes
    ObjString *string = copy_string(parser->vm,
                                    parser->previous.lexeme + 1,
                                    parser->previous.length - 2);
    emit_constant(parser, Obj_Value(string));

    Debug_Exit(parser);
}

static void boolean(Parser *parser) {
    Debug_Log(parser);
    
    TokenType type = parser->previous.type;
    emit_byte(parser, type == TOKEN_TRUE ? OP_LOAD_TRUE : OP_LOAD_FALSE);

    Debug_Exit(parser);
}

static void nil(Parser *parser) {
    Debug_Log(parser);
    
    emit_byte(parser, OP_LOAD_NIL);

    Debug_Exit(parser);
}

static void unary(Parser *parser) {
    Debug_Log(parser);
    
    TokenType operator = parser->previous.type;
    parse_precedence(parser, PREC_UNARY);

    switch (operator) {
    case TOKEN_MINUS: emit_byte(parser, OP_NEG); break;
    case TOKEN_NOT:   emit_byte(parser, OP_NOT); break;
    default:
        assert(!"invalid token type");
    }

    Debug_Exit(parser);
}

// Parsing rule table
static ParseRule rules[] = {
    { NULL,     NULL,   PREC_NONE },         // TOKEN_BREAK
    { NULL,     NULL,   PREC_NONE },         // TOKEN_COND
    { NULL,     NULL,   PREC_NONE },         // TOKEN_CONTINUE
    { NULL,     NULL,   PREC_NONE },         // TOKEN_ELSE
    { boolean,  NULL,   PREC_NONE },         // TOKEN_FALSE
    { NULL,     NULL,   PREC_NONE },         // TOKEN_FN
    { NULL,     NULL,   PREC_NONE },         // TOKEN_FOR
    { NULL,     NULL,   PREC_NONE },         // TOKEN_IF
    { NULL,     NULL,   PREC_NONE },         // TOKEN_IN
    { NULL,     NULL,   PREC_NONE },         // TOKEN_LET
    { NULL,     NULL,   PREC_NONE },         // TOKEN_MATCH
    { nil,      NULL,   PREC_NONE },         // TOKEN_NIL
    { NULL,     NULL,   PREC_NONE },         // TOKEN_RETURN
    { NULL,     NULL,   PREC_NONE },         // TOKEN_WHILE
    { boolean,  NULL,   PREC_NONE },         // TOKEN_TRUE
    { NULL,     NULL,   PREC_NONE },         // TOKEN_TYPE
    { NULL,     binary, PREC_TERM },         // TOKEN_PLUS
    { unary,    binary, PREC_TERM },         // TOKEN_MINUS
    { NULL,     binary, PREC_FACTOR },       // TOKEN_STAR
    { NULL,     binary, PREC_FACTOR },       // TOKEN_SLASH
    { NULL,     binary, PREC_FACTOR },       // TOKEN_PERCENT
    { NULL,     NULL,   PREC_NONE },         // TOKEN_DOT
    { unary,    NULL,   PREC_NONE },         // TOKEN_NOT
    { NULL,     and_,   PREC_AND },          // TOKEN_AND
    { NULL,     or_,    PREC_OR  },          // TOKEN_OR
    { NULL,     NULL,   PREC_NONE },         // TOKEN_AT
    { NULL,     NULL,   PREC_NONE },         // TOKEN_COLON_COLON
    { NULL,     binary, PREC_COMPARISON },   // TOKEN_LT
    { NULL,     binary, PREC_COMPARISON },   // TOKEN_LT_EQUAL
    { NULL,     binary, PREC_COMPARISON },   // TOKEN_GT
    { NULL,     binary, PREC_COMPARISON },   // TOKEN_GT_EQUAL
    { NULL,     NULL,   PREC_NONE },         // TOKEN_EQUAL
    { NULL,     binary, PREC_EQUALITY },     // TOKEN_EQUAL_EQUAL
    { NULL,     binary, PREC_EQUALITY },     // TOKEN_BANG_EQUAL
    { NULL,     NULL,   PREC_NONE },         // TOKEN_DO
    { NULL,     NULL,   PREC_NONE },         // TOKEN_END
    { NULL,     NULL,   PREC_NONE },         // TOKEN_PIPE
    { NULL,     NULL,   PREC_NONE },         // TOKEN_HYPHEN_LT
    { NULL,     NULL,   PREC_NONE },         // TOKEN_COMMA
    { NULL,     NULL,   PREC_NONE },         // TOKEN_SEMICOLON
    { NULL,     NULL,   PREC_NONE },         // TOKEN_COLON
    { grouping, NULL,   PREC_NONE },         // TOKEN_LEFT_PAREN
    { NULL,     NULL,   PREC_NONE },         // TOKEN_RIGHT_PAREN
    { NULL,     NULL,   PREC_NONE },         // TOKEN_LEFT_BRACE
    { NULL,     NULL,   PREC_NONE },         // TOKEN_RIGHT_BRACE
    { NULL,     NULL,   PREC_NONE },         // TOKEN_LEFT_BRACKET
    { NULL,     NULL,   PREC_NONE },         // TOKEN_RIGHT_BRACKET
    { NULL,     NULL,   PREC_NONE },         // TOKEN_IDENTIFIER
    { number,   NULL,   PREC_NONE },         // TOKEN_NUMBER
    { string,   NULL,   PREC_NONE },         // TOKEN_STRING
    { NULL,     NULL,   PREC_NONE },         // TOKEN_ERROR
    { NULL,     NULL,   PREC_NONE },         // TOKEN_EOF
};

// Return the parsing rule of a given token type.
static inline ParseRule *token_rule(TokenType type) {
    return &rules[type];
}

static void parse_precedence(Parser *parser, Precedence precedence) {
    Debug_Logf(parser, "expression(%s)", precedence_string[precedence]);
    
    advance(parser);

    ParseFn prefix = token_rule(parser->previous.type)->prefix;
    if (prefix == NULL) {
        error_previous(parser, "Unexpected token, expect expression");
        return;
    }

    prefix(parser);
    while (precedence <= token_rule(parser->current.type)->precedence) {
        advance(parser);
        token_rule(parser->previous.type)->infix(parser);
    }

    Debug_Exit(parser);
}

static inline void expression(Parser *parser) {
    parse_precedence(parser, PREC_ASSIGNMENT);
}

// Put the name in the constant table as string , and return its index.
static inline uint8_t identifier_constant(Parser *parser, Token *name) {
    const char *start = name->lexeme;
    int length = name->length;
    
    Value ident = Obj_Value(copy_string(parser->vm, start, length));
    return write_constant(parser->vm->chunk, ident);
}

static inline uint8_t parse_variable(Parser *parser, const char *error) {
    consume(parser, TOKEN_IDENTIFIER, error);
    return identifier_constant(parser, &parser->previous); 
}

static inline void define_variable(Parser *parser, uint8_t name_index) {
    emit_bytes(parser, OP_DEF_GLOBAL, name_index);
}

static inline void let_declaration(Parser *parser) {
    Debug_Log(parser);

    uint8_t index = parse_variable(parser, "expect a variable name");
    
    if (match(parser, TOKEN_EQUAL)) {
        expression(parser);
    } else {
        emit_byte(parser, OP_LOAD_NIL);
    }

    consume(parser, TOKEN_SEMICOLON,
            "expect ';' or newline after let declaration");
    define_variable(parser, index);
    
    Debug_Exit(parser);
}

// Synchronize the parser state to the next statement.
static inline void recover(Parser *parser) {
    parser->panic_mode = false;

    for (;;) {
        switch (parser->current.type) {
        case TOKEN_LET:
        case TOKEN_FN:
        case TOKEN_TYPE:
        case TOKEN_RETURN:
        case TOKEN_BREAK:
        case TOKEN_CONTINUE:
        case TOKEN_EOF:
            return;

        default:
            if (parser->previous.type == TOKEN_SEMICOLON) return;
        }
        
        advance(parser);
    }
}

static inline void declaration(Parser *parser) {
    Debug_Log(parser);

    // emit_byte(parser, OP_POP); // Pop the previous declaration value.


    if (match(parser, TOKEN_LET)) {
        let_declaration(parser);
    } else  {
        expression(parser);
        // consume(parser, TOKEN_SEMICOLON,
        //         "expect ';' or newline after expression");
    }

    if (parser->panic_mode) recover(parser);

    Debug_Exit(parser);
}

bool compile(VM *vm, const char *source, const char *file) {
    Lexer lexer;
    init_lexer(&lexer, source, file);

    Parser parser;
    parser.lexer = &lexer;
    parser.vm = vm;
    parser.had_error = false;
    parser.panic_mode = false;

#ifdef DEBUG_TRACE_PARSING
    parser.level = 0;
#endif

    advance(&parser);

    // This slot should be replaced with the final expression
    // evaluation value, if it's a statement, not an expression,
    // the value will remain nil.
    // emit_byte(&parser, OP_LOAD_NIL);

    while (!match(&parser, TOKEN_EOF)) {
        declaration(&parser);
    }

    emit_byte(&parser, OP_RETURN);

#ifdef DEBUG_DUMP_CODE
    disassemble_chunk(vm->chunk, "top-level");
#endif

    return !parser.had_error;
}
