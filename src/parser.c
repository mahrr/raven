/*
 * (parser.c | 27 Feb 19 | Ahmad Maher, Kareem hamdy)
 *
 * Raven Parser
 *
*/

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "alloc.h"
#include "ast.h"
#include "debug.h"
#include "list.h"
#include "parser.h"
#include "salloc.h"

/*** DATA ***/

typedef AST_expr (*Prefix_F)(Parser);
typedef AST_expr (*Infix_F)(Parser, AST_expr);

typedef struct Parse_error *Parse_error;

struct Parse_error {
    Token where;
    char *msg;
};

struct Parser {
    Token curr;
    Token prev;
    Token peek;
    List tokens;  /* of token */
    List errors;  /* of Parse_error */
    int been_error; /* error flag */
};

/* precedences */
typedef enum {
    LOW_PREC,
    ASSIGN_PREC,
    OR_PREC,
    AND_PREC,
    EQ_PREC,
    ORD_PREC,
    LCONS_PREC,
    CONS_PREC,
    CONC_PREC,
    SUM_PREC,
    MUL_PREC,
    UNARY_PREC,
    HIGH_PREC
} Prec;

/* a message buffer. no parse error message will
   be more than 1028 character long. */
static char error_msg[1028];


/*** HELPERS ***/

/* peek at the current token */
#define curr_token(p) (p)->curr

/* check if the current token has type 't' */
#define curr_token_is(p, t) ((p)->curr->type == (t))

/* check if the previous token has type 't' */
#define prev_token_is(p, t) ((p)->prev->type == (t))

/* check if the next token has type 't' */
#define peek_token_is(p, t) ((p)->peek->type == (t))

/* check if parser reaches the end of the token list */
#define at_end(p) curr_token_is(p, TK_EOF)

/* skip newline tokens */
#define skip_newlines(p)                            \
    while (curr_token_is(p, TK_NL)) next_token(p)

/* fetch the current token and increment the token list*/
static Token next_token(Parser p) {
    if (!at_end(p)) {
        p->prev = (Token)List_iter(p->tokens);
        p->curr = (Token)List_curr(p->tokens);
        p->peek = (Token)List_peek(p->tokens);

        return p->prev;
    }

    return curr_token(p);  /* TK_EOF */
}

/* increment if the peek token has type 't' */
static Token match_token(Parser p, TK_type t) {
    if (curr_token_is(p, t))
        return next_token(p);

    return NULL;
}

/* check if the current token not from the given token types */
static int curr_token_not(Parser p, int n, va_list ap) {
    for (int i = 0; i < n; i++) {
        TK_type t = va_arg(ap, TK_type);
        if (curr_token_is(p, t)) return 0;
    }

    return 1;
}

/* register a new error in the parser errors list */
static void reg_error(Parser p, char *msg) {
    Parse_error error = make(error, R_SECN);
    error->msg = str(msg);
    error->where = curr_token(p);
    List_append(p->errors, error);
    p->been_error = 1;
}

/* return expected token if it has type 't' then increment 
   the parser token list, or return NULL and register error 
   message if not. */
static Token expect_token(Parser p, TK_type t, char *expected) {
    if (curr_token_is(p, t)) {
        return next_token(p);   /* consume current */
    } else {
        sprintf(error_msg, "%s is expected.", expected);
        reg_error(p, error_msg);
    }
    
    return NULL;
}

/* extract the int value of TK_INT token */
static int64_t int_of(Token tok) {
    assert(tok->type == TK_INT);
    
    char *endptr;  /* for strtoll function */
    int64_t i;
    
    switch (tok->lexeme[1]) {
    case 'b':
    case 'B':
        i = strtoll(tok->lexeme + 2, &endptr, 2);
        break;
    case 'o':
    case 'O':
        i = strtoll(tok->lexeme + 2, &endptr, 8);
        break;
    default:
        i = strtoll(tok->lexeme, &endptr, 0);
        break;
    }

    assert(endptr == (tok->lexeme + tok->length));
    return i;
}

/* extract the float value of TK_FLOAT token */
static long double float_of(Token tok) {
    assert(tok->type == TK_FLOAT);
    
    char *endptr;  /* for strtold function */
    long double f = strtold(tok->lexeme, &endptr);
    
    assert(endptr == (tok->lexeme + tok->length));
    return f;
}

/* extract the string value of TK_STR token*/
static char *str_of(Token tok) {
    assert(tok->type == TK_STR);
    
    return strn(tok->lexeme + 1, tok->length - 2);
}

/* return a string representation of TK_IDENT token */
static char *ident_of(Token tok) {
    assert(tok->type == TK_IDENT);

    return strn(tok->lexeme, tok->length);
}

/* return the precedence of the token 'tok' */ 
static Prec prec_of(Token tok) {
    switch (tok->type) {
        
    case TK_LPAREN:
    case TK_LBRACKET:
    case TK_DOT:
        return HIGH_PREC;
        
    case TK_NOT:
        return UNARY_PREC;

    case TK_ASTERISK:
    case TK_SLASH:
    case TK_PERCENT:
        return MUL_PREC;

    case TK_PLUS:
    case TK_MINUS:
        return SUM_PREC;

    case TK_AT:
        return CONC_PREC;

    case TK_PIPE:
        return CONS_PREC;

    case TK_GT:
    case TK_LT:
    case TK_GT_EQ:
    case TK_LT_EQ:
        return ORD_PREC;

    case TK_EQ_EQ:
    case TK_BANG_EQ:
        return EQ_PREC;

    case TK_AND:
        return AND_PREC;

    case TK_OR:
        return OR_PREC;

    case TK_EQ:
        return ASSIGN_PREC;
        
    default:
        return LOW_PREC;
    }
}


/*** INTERNALS ***/

/** patterns nodes **/

static AST_patt pattern(Parser);
static List patterns(Parser, TK_type, TK_type, char*);

static AST_patt const_patt(Parser p) {
    Token tok = next_token(p);

    AST_patt patt = make(patt, R_SECN);

    switch (tok->type) {
    case TK_INT:
        patt->obj.i = int_of(tok);
        patt->type = INT_CPATT;
        break;

    case TK_FLOAT:
        patt->obj.f = float_of(tok);
        patt->type = FLOAT_CPATT;
        break;

    case TK_RSTR:
        patt->obj.s = str_of(tok);
        patt->type = RSTR_CPATT;
        break;

    case TK_STR:
        patt->obj.s = str_of(tok);
        patt->type = STR_CPATT;
        break;

    default:
        /* impossible but to discard compiler warnings */
        break;
    }
    
    return patt;
}

static AST_patt hash_patt(Parser p) {
    next_token(p);  /* consume '{' */

    List names = List_new(R_SECN);
    List patts = List_new(R_SECN);

    if (!match_token(p, TK_RBRACE)) {
        do {
            Token ident = expect_token(p, TK_IDENT, "field name");
            if (ident == NULL) return NULL;

            if (!expect_token(p, TK_COLON, "':'"))
                return NULL;

            AST_patt patt = pattern(p);
            if (patt == NULL) return NULL;

            List_append(names, ident_of(ident));
            List_append(patts, patt);
        } while (match_token(p, TK_COMMA));

        if (!expect_token(p, TK_RBRACE, "}"))
            return NULL;
    }

    AST_hash_patt hash = make(hash, R_SECN);
    hash->names = names;
    hash->patts = patts;
    
    AST_patt patt = make(hash_patt, R_SECN);
    patt->type = HASH_PATT;
    patt->obj.hash = hash;

    return patt;
}

static AST_patt ident_patt(Parser p) {
    Token ident = next_token(p);
    
    AST_patt patt = make(patt, R_SECN);
    patt->type = IDENT_PATT;
    patt->obj.ident = ident_of(ident);

    return patt;
}

static AST_patt list_patt(Parser p) {
    next_token(p);  /* consume '[' token */

    List patts = patterns(p, TK_COMMA, TK_RBRACKET, "]");
    if (patts == NULL) return NULL;

    AST_list_patt list = make(list, R_SECN);
    list->patts = patts;

    AST_patt patt = make(patt, R_SECN);
    patt->type = LIST_PATT;
    patt->obj.list = list;

    return patt;
}

static AST_patt pair_patt(Parser p) {
    next_token(p);  /* consume '(' token */

    AST_patt hd = pattern(p);
    if (hd == NULL) return NULL;

    if (!expect_token(p, TK_PIPE, "'|'"))
        return NULL;

    AST_patt tl = pattern(p);
    if (tl == NULL) return NULL;

    if (!expect_token(p, TK_RPAREN, "')'"))
        return NULL;

    AST_pair_patt pair = make(pair, R_SECN);
    pair->hd = hd;
    pair->tl = tl;

    AST_patt patt = make(patt, R_SECN);
    patt->type = PAIR_PATT;
    patt->obj.pair = pair;

    return patt;
}

/** expressions nodes **/

static AST_piece piece(Parser, int n, ...);
static AST_expr expression(Parser, Prec);
static List expressions(Parser, TK_type, TK_type, char*);

static AST_expr access_expr(Parser p, AST_expr object) {
    next_token(p);  /* consume 'DOT' token */

    /* ignore any newlines inside of the expressions
       (e.g. "obj.field. <nl> inner_field ") */
    skip_newlines(p);
    
    Token field = expect_token(p, TK_IDENT, "field name");
    if (field == NULL) return NULL;

    AST_access_expr access = make(access, R_SECN);
    access->field = ident_of(field);
    access->object = object;

    AST_expr expr = make(expr, R_SECN);
    expr->type = ACCESS_EXPR;
    expr->obj.access = access;

    return expr;
}

static AST_expr assign_expr(Parser p, AST_expr lvalue) {
    /* left side of '=' not an identifier */
    if (lvalue->type != IDENT_EXPR) {
        reg_error(p, "invalid assignment target");
        return NULL;
    }

    next_token(p); /* consume '=' token */

    /* ignore any newlines inside of the expressions
       (e.g. "x = y <nl> = z ") */
    skip_newlines(p);

    /* LOW_PREC is the precedence below ASSIGN_PREC which allow
       multiple assign operators to nest to the right. */
    AST_expr value = expression(p, LOW_PREC);
    if (value == NULL) return NULL;

    AST_assign_expr assign = make(assign, R_SECN);
    assign->lvalue = lvalue;
    assign->value = value;

    AST_expr expr = make(expr, R_SECN);
    expr->type = ASSIGN_EXPR;
    expr->obj.assign = assign;

    return expr;
}

static AST_expr binary_expr(Parser p, AST_expr left) {
    Token op = next_token(p); /* the operator */

    /* ignore any newlines inside of the expression
       (e.g. "1 + <nl> 1") */
    skip_newlines(p);

    AST_expr right = expression(p, prec_of(op));
    if (right == NULL) return NULL;

    AST_binary_expr binary = make(binary, R_SECN);
    binary->op = op->type;
    binary->left = left;
    binary->right = right;

    AST_expr expr = make(expr, R_SECN);
    expr->type = BINARY_EXPR;
    expr->obj.binary = binary;

    return expr;
}

static AST_expr call_expr(Parser p, AST_expr func) {
    next_token(p); /* consume '(' token */

    List args = expressions(p, TK_COMMA, TK_RPAREN, "')'");
    if (args == NULL) return NULL;

    AST_call_expr call = make(call, R_SECN);
    call->func = func;
    call->args = args;

    AST_expr expr = make(expr, R_SECN);
    expr->type = CALL_EXPR;
    expr->obj.call = call;

    return expr;
}

static AST_expr cons_expr(Parser p, AST_expr head) {
    next_token(p); /* consume '|' token */

    /* LCONS_PREC is the precedence below CONS_PREC which allow
       multiple cons operators to nest to the right. */
    AST_expr tail = expression(p, LCONS_PREC);
    if (tail == NULL) return NULL;
    
    AST_binary_expr binary = make(binary, R_SECN);
    binary->left = head;
    binary->right = tail;

    AST_expr expr = make(expr, R_SECN);
    expr->type = BINARY_EXPR;
    expr->obj.binary = binary;

    return expr;
}

static AST_expr for_expr(Parser p) {
    next_token(p);  /* consume 'for' token */

    Token name = expect_token(p, TK_IDENT, "name");
    if (name == NULL) return NULL;

    if (!expect_token(p, TK_IN, "'in'"))
        return NULL;

    AST_expr iter = expression(p, LOW_PREC);
    if (iter == NULL) return NULL;

    AST_piece body = piece(p, 1, TK_END);

    AST_for_expr for_expr = make(for_expr, R_SECN);
    for_expr->name = ident_of(name);
    for_expr->iter = iter;
    for_expr->body = body;

    AST_expr expr = make(expr, R_SECN);
    expr->type = FOR_EXPR;
    expr->obj.for_expr = for_expr;

    return expr;
}

static AST_expr group_expr(Parser p) {
    next_token(p);  /* consume '(' token */

    AST_expr gr_expr = expression(p, LOW_PREC);
    if (gr_expr == NULL) return NULL;

    if (!expect_token(p, TK_RPAREN, "')'"))
        return NULL;

    AST_group_expr group = make(group, R_SECN);
    group->expr = gr_expr;

    AST_expr expr = make(expr, R_SECN);
    expr->type = GROUP_EXPR;
    expr->obj.group = group;

    return expr;
}

static AST_expr identifier(Parser p) {
    Token ident = next_token(p);  /* the IDENT token */
    
    AST_expr expr = make(expr, R_SECN);
    expr->type = IDENT_EXPR;
    expr->obj.ident = ident_of(ident);

    return expr;
}

static AST_elif_branch elif_branch(Parser p) {
    AST_expr cond = expression(p, LOW_PREC);
    if (cond == NULL) return NULL;

    if (!expect_token(p, TK_DO, "'do'"))
        return NULL;

    AST_piece body = piece(p, 3, TK_ELIF, TK_ELSE, TK_END);

    AST_elif_branch elif = make(elif, R_SECN);
    elif->cond = cond;
    elif->body = body;

    return elif;
}

static AST_expr if_expr(Parser p) {
    next_token(p);  /* consume 'if' token */

    AST_expr cond = expression(p, LOW_PREC);
    if (cond == NULL) return NULL;

    if (!expect_token(p, TK_DO, "'do'"))
        return NULL;

    /* this is an edge case, as the delimiter of the if body
       could be TK_ELSE, T_ELIF or TK_END.*/
    AST_piece then = piece(p, 3, TK_ELSE, TK_ELIF, TK_END);

    List elifs = List_new(R_SECN);
    while (prev_token_is(p, TK_ELIF)) {
        AST_elif_branch elif = elif_branch(p);
        if (elif == NULL) return NULL;
            
        List_append(elifs, elif);
    }
    

    AST_piece alter = NULL;
    if (prev_token_is(p, TK_ELSE))
        alter = piece(p, 1, TK_END);

    AST_if_expr if_expr = make(if_expr, R_SECN);
    if_expr->cond = cond;
    if_expr->then = then;
    if_expr->elifs = elifs;
    if_expr->alter = alter;

    AST_expr expr = make(expr, R_SECN);
    expr->type = IF_EXPR;
    expr->obj.if_expr = if_expr;

    return expr;
}

static AST_expr index_expr(Parser p, AST_expr object) {
    next_token(p);  /* consume '[' token */

    AST_expr index = expression(p, LOW_PREC);
    if (index == NULL) return NULL;

    if (!expect_token(p, TK_RBRACKET, "]"))
        return NULL;

    AST_index_expr index_expr = make(index_expr, R_SECN);
    index_expr->object = object;
    index_expr->index = index;
    
    AST_expr expr = make(expr, R_SECN);
    expr->type = INDEX_EXPR;
    expr->obj.index = index_expr;

    return expr;
}

static AST_match_branch match_branch(Parser p) {
    AST_patt patt = pattern(p);
    if (patt == NULL) return NULL;

    if (!expect_token(p, TK_DASH_GT, "'->'"))
        return NULL;

    AST_match_branch branch = NULL;
    
    if (match_token(p, TK_DO)) {
        AST_piece body = piece(p, 1, TK_END);
        
        branch = make(branch, R_SECN);
        branch->type = PIECE_MATCH_BRANCH;
        branch->obj.p = body;
    } else {
        AST_expr expr = expression(p, LOW_PREC);
        if (expr == NULL) return NULL;
        
        branch = make(branch, R_SECN);
        branch->type = EXPR_MATCH_BRANCH;
        branch->obj.e = expr;
    }
    branch->patt = patt;

    return branch;
}

static AST_expr match_expr(Parser p) {
    next_token(p);  /* consume 'match' token */

    AST_expr value = expression(p, LOW_PREC);
    if (value == NULL) return NULL;

    if (!expect_token(p, TK_DO, "do"))
        return NULL;

    skip_newlines(p); /* skip newlines after 'do' */
    
    List branches = List_new(R_SECN);
    while (match_token(p, TK_CASE)) {
        AST_match_branch branch = match_branch(p);
        if (branch == NULL) return NULL;
        
        List_append(branches, branch);
        skip_newlines(p);  /* skip newlines after case branch */
    }
    
    if (!expect_token(p, TK_END, "end"))
        return NULL;

    AST_match_expr match = make(match, R_SECN);
    match->value = value;
    match->branches = branches;

    AST_expr expr = make(expr, R_SECN);
    expr->type = MATCH_EXPR;
    expr->obj.match = match;

    return expr;
}

static AST_expr unary_expr(Parser p) {
    Token op = next_token(p);  /* the unary operator */

    AST_expr operand = expression(p, LOW_PREC);
    if (operand == NULL) return NULL;

    AST_unary_expr unary = make(unary, R_SECN);
    unary->op = op->type;
    unary->operand = operand;

    AST_expr expr = make(expr, R_SECN);
    expr->type = UNARY_EXPR;
    expr->obj.unary = unary;

    return expr;
}

static AST_expr while_expr(Parser p) {
    next_token(p);  /* consume 'while' token */

    AST_expr cond = expression(p, LOW_PREC);
    if (cond == NULL) return NULL;
        
    if (!expect_token(p, TK_DO, "do"))
        return NULL;
    
    AST_piece body = piece(p, 1, TK_END);

    AST_while_expr while_expr = make(while_expr, R_SECN);
    while_expr->cond = cond;
    while_expr->body = body;

    AST_expr expr = make(expr, R_SECN);
    expr->type = WHILE_EXPR;
    expr->obj.while_expr = while_expr;

    return expr;
}

/* for 'true, false and nil' literals */
static AST_expr fixed_literal(Parser p) {
    AST_lit_expr lit = make(lit, R_SECN);
    
    if (curr_token_is(p, TK_FALSE))
        lit->type = FALSE_LIT;
    else if (curr_token_is(p, TK_TRUE))
        lit->type = TRUE_LIT;
    else
        lit->type = NIL_LIT;

    next_token(p);  /* consume the fixed */
    AST_expr expr = make(expr, R_SECN);
    expr->type = LIT_EXPR;
    expr->obj.lit = lit;

    return expr;
}

static AST_expr float_literal(Parser p) {
    Token tok = next_token(p);  /* float token */
    

    AST_lit_expr lit = make(lit, R_SECN);
    lit->type = FLOAT_LIT;
    lit->obj.f = float_of(tok);
    
    AST_expr expr = make(expr, R_SECN);
    expr->type = LIT_EXPR;
    expr->obj.lit = lit;

    return expr;
}

static AST_expr fn_literal(Parser p) {
    next_token(p); /* consume 'fn' token */

    if (!expect_token(p, TK_LPAREN, "("))
        return NULL;

    List params = patterns(p, TK_COMMA, TK_RPAREN, ")");
    if (params == NULL) return NULL;

    AST_piece body = piece(p, 1, TK_END);
    
    AST_fn_lit fn = make(fn, R_SECN);
    fn->params = params;
    fn->body = body;

    AST_lit_expr lit = make(lit, R_SECN);
    lit->type = FN_LIT;
    lit->obj.fn = fn;

    AST_expr expr = make(expr, R_SECN);
    expr->type = LIT_EXPR;
    expr->obj.lit = lit;

    return expr;
}

static AST_expr hash_literal(Parser p) {
    next_token(p);  /* consume '{' token */

    List names = List_new(R_SECN);
    List values = List_new(R_SECN);

    /* not an empty hash */
    if (!match_token(p, TK_RBRACE)) {
        do {
            Token name = expect_token(p, TK_IDENT, "field name");
            if (name == NULL) return NULL;

            if (!expect_token(p, TK_COLON, ":"))
                return NULL;
        
            AST_expr value = expression(p, LOW_PREC);
            if (value == NULL) return NULL;

            List_append(names, ident_of(name));
            List_append(values, value);
        } while (match_token(p, TK_COMMA));

        if (!expect_token(p, TK_RBRACE, "}"))
            return NULL;
    }

    AST_hash_lit hash = make(hash, R_SECN);
    hash->names = names;
    hash->values = values;

    AST_lit_expr lit = make(lit, R_SECN);
    lit->type = HASH_LIT;
    lit->obj.hash = hash;

    AST_expr expr = make(expr, R_SECN);
    expr->type = LIT_EXPR;
    expr->obj.lit = lit;

    return expr;
}

static AST_expr int_literal(Parser p) {
    Token tok = next_token(p);  /* int token */
    int64_t i = int_of(tok);
    
    AST_lit_expr lit = make(lit, R_SECN);
    lit->type = INT_LIT;
    lit->obj.i = i;

    AST_expr expr = make(expr, R_SECN);
    expr->type = LIT_EXPR;
    expr->obj.lit = lit;

    return expr;
}

static AST_expr list_literal(Parser p) {
    next_token(p);  /* consume '[' token */

    List values = expressions(p, TK_COMMA, TK_RBRACKET, "]");
    if (values == NULL) return NULL;

    AST_list_lit list = make(list, R_SECN);
    list->values = values;

    AST_lit_expr lit = make(lit, R_SECN);
    lit->type = LIST_LIT;
    lit->obj.list = list;

    AST_expr expr = make(expr, R_SECN);
    expr->type = LIT_EXPR;
    expr->obj.lit = lit;

    return expr;
}

static AST_expr str_literal(Parser p) {
    Token tok = next_token(p);  /* str token */

    AST_lit_expr lit = make(lit, R_SECN);
    lit->type = tok->lexeme[0] == '`' ? RSTR_LIT : STR_LIT;
    lit->obj.s = str_of(tok);

    AST_expr expr = make(expr, R_SECN);
    expr->type = LIT_EXPR;
    expr->obj.lit = lit;

    return expr;
}

/* return the prefix parse function of the token 'tok' */
static Prefix_F prefix_of(Token tok) {
    switch (tok->type) {
    case TK_MINUS:
    case TK_NOT:
        return unary_expr;

    case TK_INT:
        return int_literal;
        
    case TK_FLOAT:
        return float_literal;

    case TK_RSTR:
    case TK_STR:
        return str_literal;
  
    case TK_FALSE:
    case TK_TRUE:
    case TK_NIL:
        return fixed_literal;

    case TK_FN:
        return fn_literal;

    case TK_LBRACKET:
        return list_literal;

    case TK_LBRACE:
        return hash_literal;

    case TK_IDENT:
        return identifier;

    case TK_LPAREN:
        return group_expr;

    case TK_IF:
        return if_expr;

    case TK_FOR:
        return for_expr;

    case TK_WHILE:
        return while_expr;
        
    case TK_MATCH:
        return match_expr;

    default:
        return NULL;
    }
}

/* return the infix parse function of the token 'tok' */
static Infix_F infix_of(Token tok) {
    switch (tok->type) {
    case TK_PLUS:
    case TK_MINUS:
    case TK_ASTERISK:
    case TK_SLASH:
    case TK_PERCENT:
    case TK_LT:
    case TK_GT:
    case TK_EQ_EQ:
    case TK_BANG_EQ:
    case TK_LT_EQ:
    case TK_GT_EQ:
    case TK_AND:
    case TK_OR:
    case TK_AT:
        return binary_expr;

        /* '|' and '=' are right associative, so they have
           their own function instead of binary_expr */
    case TK_PIPE:
        return cons_expr;
    case TK_EQ:
        return assign_expr;
        
    case TK_LPAREN:
        return call_expr;

    case TK_LBRACKET:
        return index_expr;
        
    case TK_DOT:
        return access_expr;

    default:
        return NULL;
    }
}

/** statements nodes **/

/*
  ::NOTE::
  Statements parse functions return NULL on an error,
  so 'piece' function can notice that and synchronize
  the tokens to the next statement skipping any tokens
  came after the token that cause the parse error.
*/

static AST_stmt expr_stmt(Parser p) {
    AST_expr expr = expression(p, LOW_PREC);
    if (expr == NULL) return NULL;

    AST_expr_stmt expr_stmt = make(expr_stmt, R_SECN);
    expr_stmt->expr = expr;
    
    AST_stmt stmt = make(stmt, R_SECN);
    stmt->type = EXPR_STMT;
    stmt->obj.expr = expr_stmt;

    return stmt;
}

static AST_stmt fn_stmt(Parser p) {
    next_token(p); /* consume 'fn' token */

    Token name = expect_token(p, TK_IDENT, "name");
    if (name == NULL) return NULL;
    
    if (!expect_token(p, TK_LPAREN, "'('"))
        return NULL;

    List params = patterns(p, TK_COMMA, TK_RPAREN, ")");
    if (params == NULL) return NULL;

    AST_piece body = piece(p, 1, TK_END);

    AST_fn_stmt fn = make(fn, R_SECN);
    fn->name = ident_of(name);
    fn->params = params;
    fn->body = body;
    
    AST_stmt stmt = make(stmt, R_SECN);
    stmt->type = FN_STMT;
    stmt->obj.fn = fn;

    return stmt;
}

static AST_stmt let_stmt(Parser p) {
    next_token(p);  /* consume 'let' token */
    
    AST_patt patt = pattern(p);
    if (patt == NULL) return NULL;

    if (!expect_token(p, TK_EQ, "'='"))
        return NULL;

    AST_expr expr = expression(p, LOW_PREC);
    if (expr == NULL) return NULL;

    AST_let_stmt let = make(let, R_SECN);
    let->patt = patt;
    let->value = expr;
    
    AST_stmt stmt = make(stmt, R_SECN);
    stmt->type = LET_STMT;
    stmt->obj.let = let;

    return stmt;
}

static AST_stmt ret_stmt(Parser p) {
    next_token(p); /* consume 'return' token */

    AST_expr expr = expression(p, LOW_PREC);
    if (expr == NULL) return NULL;

    AST_ret_stmt ret = make(ret, R_SECN);
    ret->value = expr;
    
    AST_stmt stmt = make(stmt, R_SECN);
    stmt->type = RET_STMT;
    stmt->obj.ret = ret;

    return stmt;
}

static AST_stmt fixed_stmt(Parser p) {
    Token fixed = next_token(p); /* consume fixed keyword */
    
    AST_stmt stmt = make(stmt, R_SECN);
    stmt->type = FIXED_STMT;
    stmt->obj.fixed = fixed->type;

    return stmt;
}

/** main nodes **/

static AST_patt pattern(Parser p) {
    Token curr = curr_token(p);
    
    switch(curr->type) {
    case TK_LBRACE:
        return hash_patt(p);

    case TK_LBRACKET:
        return list_patt(p);

    case TK_LPAREN:
        return pair_patt(p);

    case TK_IDENT:
        return ident_patt(p);

    case TK_STR:
    case TK_INT:
    case TK_FLOAT:
        return const_patt(p);

    default:
        reg_error(p, "invalid pattern");
        return NULL;
    }
}

/* return list of zero or more AST_patt delimited by 
   'dl' token type and ended witn 'end' token type */
static List
patterns(Parser p, TK_type dl, TK_type end, char *end_name) {
    List patts = List_new(R_SECN);
    AST_patt patt;

    if (!match_token(p, end)) {
        do {
            patt = pattern(p);
            if (patt == NULL) return NULL;
            List_append(patts, patt);
        } while(match_token(p, dl));

        if (!expect_token(p, end, end_name))
            return NULL;
    }

    return patts;
}

static AST_expr expression(Parser p, Prec prec) {
    Prefix_F prefix = prefix_of(curr_token(p));
    
    if (prefix == NULL) {
        reg_error(p, "unexpected symbol");
        return NULL;
    }

    AST_expr expr = prefix(p);
    while (!at_end(p) &&
           !curr_token_is(p, TK_NL) &&
           !curr_token_is(p, TK_SEMICOLON) &&
           prec < prec_of(curr_token(p))) {
        
        Infix_F infix = infix_of(curr_token(p));
        
        if (infix == NULL)
            return expr;

        expr = infix(p, expr);
    }
    
    return expr;
}

/* return list of zero or more AST_expr delimited by 
   'dl' token type and ended witn 'end' token type */
static List
expressions(Parser p, TK_type dl, TK_type end, char *end_name) {
    List exprs = List_new(R_SECN);
    AST_expr expr;

    if (!match_token(p, end)) {
        do {
            expr = expression(p, R_SECN);
            if (expr == NULL) return NULL;
            List_append(exprs, expr);
        } while (match_token(p, dl));

        if (!expect_token(p, end, end_name))
            return NULL;
    }

    return exprs;
}

static AST_stmt statement(Parser p, int n, va_list ap) {
    AST_stmt stmt;

    switch (curr_token(p)->type) {
    case TK_FN:
        /* check first if it's 'fn_stmt' and not a 'fn_literal' */
        if (peek_token_is(p, TK_IDENT))
            stmt = fn_stmt(p);
        else
            stmt = expr_stmt(p);
        break;
            
    case TK_LET:
        stmt = let_stmt(p);
        break;
        
    case TK_RETURN:
        stmt = ret_stmt(p);
        break;
        
    case TK_CONTINUE:
    case TK_BREAK:
        stmt = fixed_stmt(p);
        break;
        
    default:
        stmt = expr_stmt(p);
        break;
    }

    /* if there is no error and not the end of the block */
    if (stmt != NULL && curr_token_not(p, n, ap)) {
        /* check for newline or semicolon, otherwise report an error */
        if (!curr_token_is(p, TK_NL) &&
            !curr_token_is(p, TK_SEMICOLON)) {
            reg_error(p, "expect ';' or newline after statement");
            return NULL;
        }
        next_token(p); /* if ';' or newline, consume it */
    }

    return stmt;
}

/* syncronize the parser token list to the start of the next statement */
static void sync(Parser p) {
    while (!at_end(p)                     &&
           !curr_token_is(p, TK_FN)       &&
           !curr_token_is(p, TK_LET)      &&
           !curr_token_is(p, TK_RETURN)   &&
           !curr_token_is(p, TK_CONTINUE) &&
           !curr_token_is(p, TK_BREAK)) {
        next_token(p);
    }
}

/* parse a block of statements until TK_EOF token or any
   token specified in the variable length argument list */
static AST_piece piece(Parser p, int n, ...) {
    va_list ap;
    List stmts = List_new(R_SECN);

    /* ignore any newlines before the beginning of the block */
    skip_newlines(p);

    va_start(ap, n);
    while (!at_end(p) && curr_token_not(p, n, ap)) {
        /* reset the argument list for 'statement' function */
        va_start(ap, n);
        AST_stmt stmt = statement(p, n, ap);
        
        if (stmt != NULL) {
            List_append(stmts, stmt);
        } else {
            /* if error occur discard any tokens left from the
               current statement, as they will produce meaningless
               error messages. */
            sync(p);
        }

        /* reset the argument list for the new loop */
        va_start(ap, n);

        /* ignore any newlines occur before the next statement */
        skip_newlines(p);
    }

    va_start(ap, n);
    if (curr_token_not(p, n, ap)) {
        reg_error(p, "'end' expected");
        return NULL;
    }
    next_token(p);  /* consume end token */

    va_end(ap);
    AST_piece piece = make(piece, R_SECN);
    piece->stmts = stmts;
    return piece;
}

/*** INTERFACE ***/

void init_parser(Parser p, List tokens) {
    p->tokens = tokens;
    p->errors = List_new(R_SECN);
    p->been_error = 0;

    p->curr = (Token)List_curr(tokens);
    p->peek = (Token)List_peek(tokens);
    p->prev = NULL;
}

Parser parser_new(List tokens, Region_N reg) {
    Parser p = make(p, reg);
    init_parser(p, tokens);
    
    return p;
}

int parser_error(Parser p) {
    return p->been_error;
}

void parser_log(Parser p, FILE *out) {
    Parse_error error;

    /* temporary! will be changed */
    while ((error = (Parse_error)List_iter(p->errors))) {
        Token t = error->where;
        fprintf(out, "syntax error: [line (%ld)] at '%.*s' : %s.\n",
                t->line,
                t->type == TK_EOF ? 3 : t->length,
                t->type == TK_EOF ? "EOF" : t->lexeme,
                error->msg);
    }
}

AST_piece parse_piece(Parser p) {
    return piece(p, 1, TK_EOF);
}

AST_stmt parse_stmt(Parser p) {
    return statement(p, 0, NULL);
}

AST_expr parse_expr(Parser p) {
    return expression(p, LOW_PREC);
}

AST_patt parse_patt(Parser p) {
    return pattern(p);
}
