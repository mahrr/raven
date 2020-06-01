#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct Local {
    Token name;
    int depth;  // -1 indicates uninitialized state
} Local;

// Context (Lexical Block) State
typedef struct Context {
    Local locals[LOCALS_LIMIT];
    int local_count;   // Number of locals in the current scope
    int scope_depth;   // Number of the surrounding blocks
} Context;

// Parser State
typedef struct {
    Lexer *lexer;     // The input, token stream
    VM *vm;           // The output chunk, bytecode stream

    Context *context; // The current scope state
    
    Token current;    // Current consumed token
    Token previous;   // Previously consumed token
    
    bool had_error;   // Error flag to stop bytecode execution later
    bool panic_mode;  // If set, any parsing error will be ignored

    // Used in continue statement.
    int inner_loop_start;
    int inner_loop_depth;

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

static inline void emit_loop(Parser *parser, int start) {
    emit_byte(parser, OP_JMP_BACK);

    // +2 for the OP_JMP_BACK 2-bytes operand.
    int offset = parser->vm->chunk->count - start + 2;
    if (offset > UINT16_MAX) {
        error_current(parser, "loop body exceeds the allowed limit");
    }

    emit_bytes(parser, (offset >> 8) & 0xff, offset & 0xff);
}

static inline void patch_jump(Parser *parser, int from) {
    // -2 because of the jmp instruction 2-bytes immediate argument
    int offset = parser->vm->chunk->count - from - 2;

    if (offset > UINT16_MAX) {
        error_current(parser, "jump offset exceeds the allowed limit");
    }

    parser->vm->chunk->opcodes[from] = (offset >> 8) & 0xff;
    parser->vm->chunk->opcodes[from + 1] = offset & 0xff;
}

static inline uint8_t make_constant(Parser *parser, Value value) {
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

// Put the name in the constant table as string , and return its index.
static inline uint8_t identifier_constant(Parser *parser, Token *name) {
    const char *start = name->lexeme;
    int length = name->length;
    
    Value ident = Obj_Value(copy_string(parser->vm, start, length));
    return write_constant(parser->vm->chunk, ident);
}

/** Parser State **/

static inline void init_parser(Parser *parser, Lexer *lexer, VM *vm) {
    parser->lexer = lexer;
    parser->vm = vm;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->inner_loop_start = -1;
    parser->inner_loop_depth = -1;
    
#ifdef DEBUG_TRACE_PARSING
    parser->level = 0;
#endif
}

static inline void init_context(Parser *parser, Context *context) {
    context->local_count = 0;
    context->scope_depth = 0;
    parser->context = context;
}

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

static inline void add_local(Parser *parser, Token name) {
    Context *context = parser->context;

    if (context->local_count == LOCALS_LIMIT) {
        error_previous(parser, "Function locals limit is exceeded");
        return;
    }
    
    Local *local = &context->locals[context->local_count++];
    local->name = name;
    local->depth = -1; // Uninitialized
}

static inline void begin_scope(Parser *parser) {
    parser->context->scope_depth++;
}

static void unwind_stack(Parser *parser, int depth) {
    Context *context = parser->context;
    int local_count = 0;
    
    for (int i = context->local_count - 1; i >= 0; i--) {
        if (context->locals[i].depth < depth) {
            break;
        }

        local_count++;
    }

    context->local_count -= local_count;
    if (local_count != 0) {
        emit_bytes(parser, OP_POPN, (uint8_t)local_count);
    }
}

static inline void end_scope(Parser *parser, bool loading) {
    unwind_stack(parser, parser->context->scope_depth);
    
    // Push the value of the last expression in the block.
    if (loading) emit_byte(parser, OP_LOAD);
    parser->context->scope_depth--;
}

static inline bool same_identifier(Token *a, Token *b) {
    if (a->length != b->length) return false;
    return memcmp(a->lexeme, b->lexeme, a->length) == 0;
}

// TODO: consider using a hash map for faster lookup.
static inline int resolve_local(Context *context, Token *name) {
    for (int i = context->local_count - 1; i >= 0; i--) {
        Local *local = &context->locals[i];
        
        // Find an initialized local variable with this name.
        // The "local->depth != -1" test skips the edge case:
        //   let x = x;
        // But in the same time, allow same name shadowing:
        //   do
        //     let x = 1;
        //     do
        //       let x = x + x; // x -> 2
        //       ...
        //     end
        //     ...
        //   end
        if (local->depth != -1 && same_identifier(name, &local->name)) {
            return i;
        }
    }

    // Not Found
    return -1;
}

/** Parsing **/

static void parse_precedence(Parser*, Precedence);
static void expression(Parser*);
static void declaration(Parser*);
static inline ParseRule *token_rule(TokenType);

static void assignment(Parser *parser) {
    Debug_Log(parser);
    
    Chunk *chunk = parser->vm->chunk;

    // Check if the left hand side was an identifier, it's kind of a hack.
    // TODO: experiemnt with alternative approaches.
    if (chunk->count < 2 ||
        (chunk->opcodes[chunk->count-2] != OP_GET_GLOBAL &&
         chunk->opcodes[chunk->count-2] != OP_GET_LOCAL)) {
        error_previous(parser, "invalid assignment target");
        return;
    }

    // Get the name, or slot index and extract the corresponding
    // set instruction, and then discard the get instruction.
    uint8_t index = chunk->opcodes[chunk->count-1];
    uint8_t set_op = chunk->opcodes[chunk->count-2] - 1;
    chunk->count -= 2;

    
    // Not PREC_ASSIGNMENT + 1, sicne assignment is right associated.
    parse_precedence(parser, PREC_ASSIGNMENT);
    emit_bytes(parser, set_op, index);

    Debug_Exit(parser);
}

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
    // Operator Flow:
    //
    // [Left Operand]
    // OP_JMP_FALSE   ---.
    // OP_POP            |
    // [Right Operand]   |
    // ....    <----------
    
    Debug_Log(parser);

    int jump = emit_jump(parser, OP_JMP_FALSE);

    emit_byte(parser, OP_POP);
    parse_precedence(parser, PREC_AND + 1);

    patch_jump(parser, jump);

    Debug_Exit(parser);
}

// TODO: test the performance difference with OP_JMP_TRUE instruction.
static void or_(Parser *parser) {
    // Operator Flow:
    //
    // [Left Operand]
    // OP_JMP_FALSE   ---.
    // OP_JMP         ---|--.
    // OP_POP  <----------  |
    // [Right Operand]      |
    // ....    <-------------
    
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

// TODO: abstract the block parsing function in a clean way.
static void if_block(Parser *parser) {
    Debug_Log(parser);

    begin_scope(parser);

    while (!check(parser, TOKEN_END) &&
           !check(parser, TOKEN_EOF) &&
           !check(parser, TOKEN_ELSE)) {
        declaration(parser);
    }

    if (!match(parser, TOKEN_ELSE)) {
        consume(parser, TOKEN_END, "expect closing 'end' after if block");
    }
    
    end_scope(parser, true);
    
    Debug_Exit(parser);
}

static void loop_block(Parser *parser) {
    Debug_Log(parser);
       
    begin_scope(parser);

    while (!check(parser, TOKEN_END) && !check(parser, TOKEN_EOF)) {
        declaration(parser);
    }

    consume(parser, TOKEN_END, "expect closing 'end' after loop block");

    end_scope(parser, false);
    
    Debug_Exit(parser);
}

static void block(Parser *parser) {
    Debug_Log(parser);

    begin_scope(parser);

    while (!check(parser, TOKEN_END) && !check(parser, TOKEN_EOF)) {
        declaration(parser);
    }

    consume(parser, TOKEN_END, "expect closing 'end' after block");
    
    end_scope(parser, true);
    
    Debug_Exit(parser);
}

static void cond(Parser *parser) {
    // Cond Control Flow:
    //
    // Condition (1)
    // OP_JMP_POP_FALSE  ---.
    //                      |
    // Expression (1)       |
    // OP_JMP            --------.
    //                      |    |
    // [Condition (2)]   <---    |
    // OP_JMP_POP_FALSE  ----    |
    //                      |    |
    // [Expression (2)]     |    |
    // OP_JMP            --------|
    //                      |    |
    // [Condition (3)]   <---    |
    // OP_JMP_POP_FALSE  ---.    |
    //                      |    |
    // [Expression (3)]     |    |
    // OP_JMP            --------|
    //                      |    |
    // ....                 .    .
    // ....                 .    .
    //                      |    |
    // [Condition (n)]   <---    |
    // OP_JMP_POP_FALSE  ---.    |
    //                      |    |
    // [Expression (n)]     |    |
    // OP_JMP            --------|
    //                      |    |
    // OP_LOAD_NIL <---------    |
    // ....        <--------------

    Debug_Log(parser);
    consume(parser, TOKEN_COLON, "expect ':' after cond");

    int cases_exit[COND_LIMIT];
    int cases_count = 0;

    do {
        if (cases_count == COND_LIMIT) {
            error_previous(parser, "cond cases exceeds the allowd limit");
            break;
        }
        
        // Condition
        expression(parser);
        consume(parser, TOKEN_ARROW, "expect '->' after expression");
        int next_case = emit_jump(parser, OP_JMP_POP_FALSE);

        // Expression
        expression(parser);
        cases_exit[cases_count++] = emit_jump(parser, OP_JMP);
        
        patch_jump(parser, next_case);
    } while (match(parser, TOKEN_COMMA));

    // If all conditions evaluate to false.
    emit_byte(parser, OP_LOAD_NIL);
    
    for (int i = 0; i < cases_count; i++) {
        patch_jump(parser, cases_exit[i]);
    }

    consume(parser, TOKEN_END, "expect 'end' after cond cases");
    
    Debug_Exit(parser);
}

static void if_(Parser *parser) {
    // If Control Flow:
    //
    // Condition
    // OP_JMP_POP_FALSE   ----.
    //                        |
    // Then Block             |
    // OP_JMP             --------.
    //                        |   |
    // [Else Block]    <-------   | 
    // ....            <-----------
    
    Debug_Log(parser);
    
    expression(parser); // Condition
    consume(parser, TOKEN_DO, "expect 'do' after if condition");

    int then_jump = emit_jump(parser, OP_JMP_POP_FALSE);  // ---. false
    if_block(parser);                                     //    |
                                                          //    |
    int else_jump = emit_jump(parser, OP_JMP);            // ---|--. true
                                                          //    |  |
    patch_jump(parser, then_jump);                        // <---  |
                                                          //       |
    if (parser->previous.type == TOKEN_ELSE) {            //       |
        block(parser);                                    //       |
    }                                                     //       |
                                                          //       |
    patch_jump(parser, else_jump);                        // <------

    Debug_Exit(parser);
}

static void while_(Parser *parser) {
    // While Control Flow
    //
    // Condition       <----------.
    // OP_JMP_POP_FALSE   -----.  |
    //                         |  |
    // [continue]              |  |
    // OP_JMP_BACK        --------|
    //                         |  |
    // Loop Body               |  |
    // OP_JMP_BACK        ---------
    //                         |
    // ....            <--------
    
    Debug_Log(parser);

    // Register the surrounding loop state.
    int previous_inner_loop_start = parser->inner_loop_start;
    int previous_inner_loop_depth = parser->inner_loop_depth;

    int loop_start = parser->vm->chunk->count;            // <-----.
                                                          //       |
    // Push the current loop state                        //       |
    parser->inner_loop_start = loop_start;                //       |
    parser->inner_loop_depth = parser->context->scope_depth;//     |
                                                          //       |
    expression(parser); // Condition                      //       |
                                                          //       |
    consume(parser, TOKEN_DO, "expect 'do' after while condition");
                                                          //       |
    int exit_jump = emit_jump(parser, OP_JMP_POP_FALSE);  // ---.  |
                                                          //    |  |
    loop_block(parser);                                   //    |  |
                                                          //    |  |
    emit_loop(parser, loop_start);                        // -------
                                                          //    |
    patch_jump(parser, exit_jump);                        // <---

    // The resulting expression of a loop is always nil.
    emit_byte(parser, OP_LOAD_NIL);

    // Pop the current loop state.
    parser->inner_loop_start = previous_inner_loop_start;
    parser->inner_loop_depth = previous_inner_loop_depth;
    
    Debug_Exit(parser);
}

static void grouping(Parser *parser) {
    Debug_Log(parser);
    
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN,
            "Expect closing ')' after group expression");

    Debug_Exit(parser);
}

static void identifier(Parser *parser) {
    Debug_Log(parser);

    uint8_t get_op;
    int index = resolve_local(parser->context, &parser->previous);

    if (index != -1) {
        get_op = OP_GET_LOCAL;
    } else {
        index = identifier_constant(parser, &parser->previous);
        get_op = OP_GET_GLOBAL;
    }
    
    emit_bytes(parser, get_op, index);

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
    { NULL,       NULL,       PREC_NONE },         // TOKEN_BREAK
    { cond,       NULL,       PREC_NONE },         // TOKEN_COND
    { NULL,       NULL,       PREC_NONE },         // TOKEN_CONTINUE
    { NULL,       NULL,       PREC_NONE },         // TOKEN_ELSE
    { boolean,    NULL,       PREC_NONE },         // TOKEN_FALSE
    { NULL,       NULL,       PREC_NONE },         // TOKEN_FN
    { NULL,       NULL,       PREC_NONE },         // TOKEN_FOR
    { if_,        NULL,       PREC_NONE },         // TOKEN_IF
    { NULL,       NULL,       PREC_NONE },         // TOKEN_IN
    { NULL,       NULL,       PREC_NONE },         // TOKEN_LET
    { NULL,       NULL,       PREC_NONE },         // TOKEN_MATCH
    { nil,        NULL,       PREC_NONE },         // TOKEN_NIL
    { NULL,       NULL,       PREC_NONE },         // TOKEN_RETURN
    { while_,     NULL,       PREC_NONE },         // TOKEN_WHILE
    { boolean,    NULL,       PREC_NONE },         // TOKEN_TRUE
    { NULL,       NULL,       PREC_NONE },         // TOKEN_TYPE
    { NULL,       binary,     PREC_TERM },         // TOKEN_PLUS
    { unary,      binary,     PREC_TERM },         // TOKEN_MINUS
    { NULL,       binary,     PREC_FACTOR },       // TOKEN_STAR
    { NULL,       binary,     PREC_FACTOR },       // TOKEN_SLASH
    { NULL,       binary,     PREC_FACTOR },       // TOKEN_PERCENT
    { NULL,       NULL,       PREC_NONE },         // TOKEN_DOT
    { unary,      NULL,       PREC_NONE },         // TOKEN_NOT
    { NULL,       and_,       PREC_AND },          // TOKEN_AND
    { NULL,       or_,        PREC_OR  },          // TOKEN_OR
    { NULL,       NULL,       PREC_NONE },         // TOKEN_AT
    { NULL,       NULL,       PREC_NONE },         // TOKEN_COLON_COLON
    { NULL,       binary,     PREC_COMPARISON },   // TOKEN_LESS
    { NULL,       binary,     PREC_COMPARISON },   // TOKEN_LESS_EQUAL
    { NULL,       binary,     PREC_COMPARISON },   // TOKEN_GREATER
    { NULL,       binary,     PREC_COMPARISON },   // TOKEN_GREATER_EQUAL
    { NULL,       assignment, PREC_ASSIGNMENT },   // TOKEN_EQUAL
    { NULL,       binary,     PREC_EQUALITY },     // TOKEN_EQUAL_EQUAL
    { NULL,       binary,     PREC_EQUALITY },     // TOKEN_BANG_EQUAL
    { block,      NULL,       PREC_NONE },         // TOKEN_DO
    { NULL,       NULL,       PREC_NONE },         // TOKEN_END
    { NULL,       NULL,       PREC_NONE },         // TOKEN_PIPE
    { NULL,       NULL,       PREC_NONE },         // TOKEN_ARROW
    { NULL,       NULL,       PREC_NONE },         // TOKEN_COMMA
    { NULL,       NULL,       PREC_NONE },         // TOKEN_SEMICOLON
    { NULL,       NULL,       PREC_NONE },         // TOKEN_COLON
    { grouping,   NULL,       PREC_NONE },         // TOKEN_LEFT_PAREN
    { NULL,       NULL,       PREC_NONE },         // TOKEN_RIGHT_PAREN
    { NULL,       NULL,       PREC_NONE },         // TOKEN_LEFT_BRACE
    { NULL,       NULL,       PREC_NONE },         // TOKEN_RIGHT_BRACE
    { NULL,       NULL,       PREC_NONE },         // TOKEN_LEFT_BRACKET
    { NULL,       NULL,       PREC_NONE },         // TOKEN_RIGHT_BRACKET
    { identifier, NULL,       PREC_NONE },         // TOKEN_IDENTIFIER
    { number,     NULL,       PREC_NONE },         // TOKEN_NUMBER
    { string,     NULL,       PREC_NONE },         // TOKEN_STRING
    { NULL,       NULL,       PREC_NONE },         // TOKEN_ERROR
    { NULL,       NULL,       PREC_NONE },         // TOKEN_EOF
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

static void expression(Parser *parser) {
    parse_precedence(parser, PREC_ASSIGNMENT);
}

static void declare_variable(Parser *parser) {
    Context *context = parser->context;
    if (context->scope_depth == 0) return; // Global Scope

    Token *name = &parser->previous;
    for (int i = context->local_count - 1; i >= 0; i--) {
        Local *local = &context->locals[i];

        if (local->depth == -1 || local->depth < context->scope_depth) {
            break; // Not previously declared in the current scope.
        }

        if (same_identifier(name, &local->name)) {
            error_previous(parser, "This name is already declared.");
        }
    }
    
    add_local(parser, *name);
}

static void define_variable(Parser *parser, uint8_t name_index) {
    Context *context = parser->context;

    // Local Scope?
    if (context->scope_depth > 0) {
        // Initialize the most declared local variable.
        int last_index = context->local_count - 1;
        context->locals[last_index].depth = context->scope_depth;
        return;
    }
    
    emit_bytes(parser, OP_DEF_GLOBAL, name_index);
}

static uint8_t parse_variable(Parser *parser, const char *error) {
    consume(parser, TOKEN_IDENTIFIER, error);

    declare_variable(parser);
    if (parser->context->scope_depth > 0) return 0;
    
    return identifier_constant(parser, &parser->previous); 
}

static void let_declaration(Parser *parser) {
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

static void continue_statement(Parser *parser) {
    Debug_Log(parser);

    // In a loop?
    if (parser->inner_loop_start == -1) {
        error_previous(parser, "use of continue outside a loop");
    }

    consume(parser, TOKEN_SEMICOLON, "expect ';' after continue");

    unwind_stack(parser, parser->inner_loop_depth);
    emit_loop(parser, parser->inner_loop_start);

    Debug_Exit(parser);
}

// Synchronize the parser state to the next statement.
static void recover(Parser *parser) {
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

static void declaration(Parser *parser) {
    Debug_Log(parser);

    if (match(parser, TOKEN_LET)) {
        let_declaration(parser);
    } else if (match(parser, TOKEN_CONTINUE)) {
        continue_statement(parser);
    } else {
        expression(parser);
        // consume(parser, TOKEN_SEMICOLON,
        //         "expect ';' or newline after expression");
        emit_byte(parser, OP_STORE);
    }

    if (parser->panic_mode) recover(parser);

    Debug_Exit(parser);
}

bool compile(VM *vm, const char *source, const char *file) {
    Lexer lexer;
    init_lexer(&lexer, source, file);

    Parser parser;
    init_parser(&parser, &lexer, vm);

    Context context;
    init_context(&parser, &context);

    advance(&parser);

    while (!match(&parser, TOKEN_EOF)) {
        declaration(&parser);
    }

    emit_byte(&parser, OP_RETURN);

#ifdef DEBUG_DUMP_CODE
    disassemble_chunk(vm->chunk, "top-level");
#endif

    return !parser.had_error;
}
