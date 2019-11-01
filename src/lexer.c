/*
 * (lexer.c | 22 Nov 18 | Ahmad Maher)
 *
 * Raven Lexer
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "error.h"
#include "lexer.h"
#include "strutil.h"
#include "token.h"

/* hardcoded lexical errors messages */
#define MSG_UNREC "unrecognize syntax"
#define MSG_UNTER "unterminated string"
#define MSG_INVLD "invalid escape sequence"
#define MSG_MALSC "malformed scientific notation"

/** INTERNALS **/

/* checks if the lexer reached the end of the source */
#define at_end(l) (*((l)->current) == '\0')

/* consumes the current char, and returns it */
#define cons_char(l) at_end(l) ? '\0' : *(l)->current++

/* returns the current char without consuming it */
#define peek_char(l) *(l)->current

/* returns the next char without consuming it */
#define peek_next(l) at_end(l) ? '\0' : *((l)->current+1)

/* returns the previous consumed character */
#define prev_char(l) *((l)->current-1)

/* consume the current char if it was c */
#define match_char(l, c)                            \
    (c) == *(l)->current ? ((l)->current++, 1) : 0

/* token struct literal on the fly */
#define new_token(l, type)                          \
    (Token){type,                                   \
            l->file,                                \
            l->fixed,                               \
            l->current - l->fixed,                  \
            l->line}

/*
 * register a lexing (syntax) error at token where,
 * with specific error message.
*/
static void reg_error(Lexer *l, Token *where, const char *msg) {
    Err err = (Err){SYNTAX_ERR, *where, msg};
    l->been_error = 1;
    ARR_ADD(&l->errors, err);
}

/* 
 * add token to the lexer tokens array, 
 * and return its address.
*/
static Token *add_token(Lexer *l, TK_type type) {
    ARR_ADD(&l->tokens, new_token(l, type));
    return &l->tokens.elems[l->tokens.len-1];
}

/*
 * register an lexing error with specified message,
 * and add an error token to the lexer tokens array.
 * it returns the address of the error token.
*/
static Token *add_error(Lexer *l, char *msg) {
    Token *err = add_token(l, TK_ERR);
    reg_error(l, err, msg);
    return err;
}

/* skips the short one-line comments */
static void line_comment(Lexer *l) {
    while (!at_end(l) && peek_char(l) != '\n')
        cons_char(l);
    
    /* skip the newline at the end of the comment,
       as not to consume it as token */
    if (peek_char(l) == '\n') {
        l->line++;
        cons_char(l);
    }
}

/* skips the long comments */
static void long_comment(Lexer *l) {    
    cons_char(l); /* '#' */
    cons_char(l); /* '-' */

    int unclosed = 1; /* number of unclosed '#-' */

    while (unclosed) {
        while (!at_end(l) &&
               ((peek_char(l) != '-') || (peek_next(l) != '#'))) {
            cons_char(l);

            /* '#-' found */
            if ((peek_char(l) == '#') && (peek_next(l) == '-'))
                unclosed++;
        
            if (peek_char(l) == '\n')
                l->line++;
        }

        /* '-#' found */
        if (!at_end(l)) {
            unclosed--;
            cons_char(l); /* '-' */
            cons_char(l); /* '#' */
        } else {
            return; /* unterminated long comment is not an error */
        }
    }

    if (peek_char(l) == '-') {
        cons_char(l);  /* '-' */
        cons_char(l);  /* '#' */
    }
}

static void skip_whitespace(Lexer *l) {
    for (;;) {
        switch (peek_char(l)) {
        case ' ':
        case '\t':
        case '\r':
            cons_char(l);
            break;
        case '#':
            if (peek_next(l) == '-')
                long_comment(l);
            else
                line_comment(l);
            break;
        default:
            return;
        }
    }
}

/* 
 * check if the rest of the keyword match with 
 * the rest of the current token being consumed.
 * rest is encoded string, so it's okay to use
 * (sizeof(rest)-1) for its length.
*/
#define match_keyword(l, rest)                              \
    ((sizeof(rest) - 1) == (l->current - l->fixed - 1) &&   \
     !strncmp(rest, l->fixed + 1, sizeof(rest) - 1))

/* consumes a keywords if matches, otherwise an identifiers */
static Token *cons_ident(Lexer *l) {
    /* extract the whole token first */
    char start_ch = prev_char(l);

    /* consume characters until end of a name */
    while (!at_end(l) &&
           (isalnum(peek_char(l)) || peek_char(l) == '_'))
        cons_char(l);

    /* probably faster than hash table but need to be tested though */
    switch (start_ch) {
    case 'a':
        if (match_keyword(l, "nd"))
            return add_token(l, TK_AND);
        break;
        
    case 'b':
        if (match_keyword(l, "reak"))
            return add_token(l, TK_BREAK);
        break;
        
    case 'c':
        if (match_keyword(l, "ase"))
            return add_token(l, TK_CASE);
        else if (match_keyword(l, "ond"))
            return add_token(l, TK_COND);
        else if (match_keyword(l, "ontinue"))
            return add_token(l, TK_CONTINUE);
        break;
        
    case 'd':
        if (match_keyword(l, "o"))
            return add_token(l, TK_DO);
        break;
        
    case 'e':
        if (match_keyword(l, "lif"))
            return add_token(l, TK_ELIF);
        else if (match_keyword(l, "lse"))
            return add_token(l, TK_ELSE);
        else if (match_keyword(l, "nd"))
            return add_token(l, TK_END);
        break;
        
    case 'f':
        if (match_keyword(l, "alse"))
            return add_token(l, TK_FALSE);
        else if (match_keyword(l, "n"))
            return add_token(l, TK_FN);
        else if (match_keyword(l, "or"))
            return add_token(l, TK_FOR);
        break;
        
    case 'i':
        if (match_keyword(l, "f"))
            return add_token(l, TK_IF);
        else if (match_keyword(l, "n"))
            return add_token(l, TK_IN);
        break;
        
    case 'l':
        if (match_keyword(l, "et"))
            return add_token(l, TK_LET);
        break;
        
    case 'm':
        if (match_keyword(l, "atch"))
            return add_token(l, TK_MATCH);
        break;
        
    case 'n':
        if (match_keyword(l, "il"))
            return add_token(l, TK_NIL);
        else if (match_keyword(l, "ot"))
            return add_token(l, TK_NOT);
        break;
        
    case 'o':
        if (match_keyword(l, "r"))
            return add_token(l, TK_OR);
        break;
        
    case 'r':
        if (match_keyword(l, "eturn"))
            return add_token(l, TK_RETURN);
        break;
        
    case 't':
        if (match_keyword(l, "ype"))
            return add_token(l, TK_TYPE);
        else if (match_keyword(l, "rue"))
            return add_token(l, TK_TRUE);
        break;
        
    case 'w':
        if (match_keyword(l, "hile"))
            return add_token(l, TK_WHILE);
        break;
    }

    /* not a keyword */
    return add_token(l, TK_IDENT);  
}

#define is_hexa_digit(n)                        \
    (n >= '0' && n <= '9') ||                   \
    (n >= 'a' && n <= 'f') ||                   \
    (n >= 'A' && n <= 'F')

#define is_octal_digit(n)                       \
    (n >= '0' && n <= '7')

#define is_bin_digit(n)                         \
    (n == '0' || n == '1')

/* consume numeric tokens */
Token *cons_num(Lexer *l) {
    
    char start_ch = prev_char(l);
    int is_float = 0;  /* a flag for floating point numbers */

    /* case of hexadecimal, octal and binary numbers */
    if (start_ch == '0') {
        switch (peek_char(l)) {
        case 'x':
        case 'X':
            cons_char(l);
            while (is_hexa_digit(peek_char(l)))
                cons_char(l);
            return add_token(l, TK_INT);
        case 'o':
        case 'O':
            cons_char(l);
            while (is_octal_digit(peek_char(l)))
                cons_char(l);
            return add_token(l, TK_INT);
        case 'b':
        case 'B':
            cons_char(l);
            while(is_bin_digit(peek_char(l)))
                cons_char(l);
            return add_token(l, TK_INT);
        }
    }

    /* decimal form */
    while (!at_end(l) && isdigit(peek_char(l)))
        cons_char(l);

    /* decimal point form */
    if (peek_char(l) == '.' && isdigit(peek_next(l))) {
        is_float = 1;
        cons_char(l);  /* the decimal point */
        while (!at_end(l) && isdigit(peek_char(l)))
            cons_char(l);
    }

    /* scientific notation */
    if ((peek_char(l) == 'e' || peek_char(l) == 'E')
        && (isdigit(peek_next(l))
            || (peek_next(l) == '+')
            || (peek_next(l) == '-'))) {
        is_float = 1;
        
        cons_char(l);  /* consume 'e/E' */
        cons_char(l);  /* consume '+/-/<digit> */
        
        /* malformed scientific notation (ex. '1e+') */
        if (!isdigit(prev_char(l)) && !isdigit(peek_char(l)))
            return add_error(l, MSG_MALSC);
        
        while (!at_end(l) && isdigit(peek_char(l)))
            cons_char(l);
    }

    return add_token(l, is_float ? TK_FLOAT : TK_INT);
}

/* consumes escaped strings */
static Token *cons_str(Lexer *l) {
    /* single or double quote */
    char quote = prev_char(l);

    /* the beginning of the string */
    /* char *str_start = (char*)((l->current)-1); */

    /* calculate the string size making sure that the string
       doesn't terminate on an escaped single or double quotes */
    while (!at_end(l) && peek_char(l) != quote && peek_char(l) != '\n') {
        if (peek_char(l) == '\\') {
            cons_char(l);  /* the back slash */
            if (peek_char(l) == '\n')
                break;
            cons_char(l); /* the char after back slash */
            continue;
        }
        
        cons_char(l);
    }
    
    if (at_end(l) || peek_char(l) == '\n')
        return add_error(l, MSG_UNTER);

    cons_char(l);  /* consume the final quote */
    
    /* char *str_end = (char*)l->current; */
    /* int size = str_end - str_start; */
    int size = l->current - l->fixed;

    
    /* check for escaping errors */
    char *buf = malloc(size);  /* escape string buffer */
    char *end;
    strescp(l->fixed, buf, size, &end);
    free(buf);
    
    /* escape error */
    if (end != NULL) {
        Token *err = add_token(l, TK_ERR);
        /* the lexeme point to the escape error,
         * not the start of the token. */
        err->lexeme = end;
        err->length = l->current - end;
        reg_error(l, err, MSG_INVLD);
        return err;
    }
    
    /* note that the used lexeme is the original unescaped
       string, the actual escaping happen in the runtime. */
    return add_token(l, TK_STR);
}

/* consume raw strings without any escaping */
static Token *cons_rstr(Lexer *l) {
    char quote = prev_char(l);

    while (!at_end(l) && peek_char(l) != quote) {
        if (peek_char(l) == '\n') l->line++;
        cons_char(l);
    }

    if (at_end(l))
        return add_error(l, MSG_UNTER);

    cons_char(l); /* the final quote */

    return add_token(l, TK_RSTR);
}

/* consume the current token, then return it. */
static Token *consume(Lexer *l) {    
    /* ignore any whitespace characters */
    skip_whitespace(l);

    l->fixed = l->current;
    char c = cons_char(l);
     
    if (isalpha(c) || c == '_') return cons_ident(l);
    if (isdigit(c)) return cons_num(l);

    switch(c) {      
    /* one character tokens */
    case '@':
        return add_token(l, TK_AT);
    case '|':
        return add_token(l, TK_PIPE);
    case '+':
        return add_token(l, TK_PLUS);
    case '*':
        return add_token(l, TK_ASTERISK);
    case '/':
        return add_token(l, TK_SLASH);
    case '%':
        return add_token(l, TK_PERCENT);
    case '(':
        return add_token(l, TK_LPAREN);
    case ')':
        return add_token(l, TK_RPAREN);
    case '{':
        return add_token(l, TK_LBRACE);
    case '}':
        return add_token(l, TK_RBRACE);
    case '[':
        return add_token(l, TK_LBRACKET);
    case ']':
        return add_token(l, TK_RBRACKET);
    case ',':
        return add_token(l, TK_COMMA);
    case '.':
        return add_token(l, TK_DOT);
    case ':':
        return add_token(l, TK_COLON);
    case ';':
        return add_token(l, TK_SEMICOLON);
        
    /* possible two character tokens */
    case '<':
        if (match_char(l, '='))
            return add_token(l, TK_LT_EQ);
        return add_token(l, TK_LT);
        
    case '>':
        if (match_char(l, '='))
            return add_token(l, TK_GT_EQ);
        return add_token(l, TK_GT);

    case '=':
        if (match_char(l, '='))
            return add_token(l, TK_EQ_EQ);
        return add_token(l, TK_EQ);

    case '-':
        if (match_char(l, '>'))
            return add_token(l, TK_DASH_GT);
        return add_token(l, TK_MINUS);

    case '!':
        if (match_char(l, '='))
            return add_token(l, TK_BANG_EQ);
        break;

    case '"':
    case '\'':
        return cons_str(l);

    case'`':
        return cons_rstr(l);
            
    case '\n': {
        Token *tok = add_token(l, TK_NL);
        /* to avoid printing newlines at debugging */
        tok->length = 0;

        l->line++;
        return tok;
    }
        
    case '\0':
    case EOF:
        return add_token(l, TK_EOF);
    }

    return add_error(l, MSG_UNREC);
}

/** INTERFACE **/

char *scan_file(const char *file) {
    FILE *f = fopen(file, "rb");
    if (f == NULL) return NULL;
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f)+1;
    char *buff = malloc(size);

    fseek(f, 0, SEEK_SET);
    if (fread(buff, 1, size, f) == 0)
        if (ferror(f))
            fatal_err(1, "Fatal: can't read '%s'", file);
    
    fclose(f);

    return buff;
}

void init_lexer(Lexer *lexer, const char *src, const char *file) {
    lexer->current = src;
    lexer->fixed = src;
    lexer->file = file;
    lexer->line = 1;
    lexer->been_error = 0;
    ARR_INITC(&lexer->tokens, Token, 64);
    ARR_INIT(&lexer->errors, Err);
}

void free_lexer(Lexer *lexer) {
    ARR_FREE(&lexer->tokens);
    ARR_FREE(&lexer->errors);
}

Token *cons_token(Lexer *lexer) {
    return consume(lexer);
}

Token *cons_tokens(Lexer *lexer) {
    Token *tok = consume(lexer);
    while (tok->type != TK_EOF)
        tok = consume(lexer);

    return lexer->tokens.elems;
}
