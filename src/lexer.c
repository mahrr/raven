/*
 * (lexer.c | 22 Nov 18 | Ahmad Maher)
 *
 * Raven lexer.
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "lexer.h"
#include "alloc.h"
#include "salloc.h"
#include "strutil.h"
#include "list.h"


struct Lexer {
    const char *current; /* the current unconsumed char in the source */
    const char *fixed;   /* the start of the current token */
    const char *file;    /* the source file name */
    long line;           /* the current line number */
    Region_N reg;        /* the allocation region for the tokens */
    int been_error;      /* lexing error flag */
    List tokens;       /* list of the consumed tokens */
    List errors;       /* list of the tokens errors */
};

/* hardcoded lexical errors messages */
#define MSG_UNREC "unrecognize syntax"
#define MSG_UNTER "unterminated string"
#define MSG_INVLD "invalid escape sequence"
#define MSG_MALSC "malformed scientific notation"

/* allocate a new token using the lexer current state. */
static Token new_token(Lexer l, TK_type type, char *err_msg) {
    Token tok = make(tok, l->reg);
    tok->type = type;
    tok->lexeme = l->fixed;
    tok->length = l->current - l->fixed;
    tok->file = l->file;
    tok->line = l->line;
    tok->err_msg = err_msg;

    return tok;
}

/* small wrappers around new_token for less typing. */
#define error_token(l, msg) new_token(l, TK_ERR, msg)
#define valid_token(l, type) new_token(l, type, NULL)

/* checks if the lexer reached the end of the source */
#define at_end() (*(l->current) == '\0')

/* consumes the current char, and returns it */
#define cons_char() at_end() ? '\0' : *l->current++

/* returns the current char without consuming it */
#define peek_char() *l->current

/* return the next char without consuming it */
#define peek_next() at_end() ? '\0' : *(l->current+1)

/* return the last consumed character */
#define prev_char() *(l->current-1)

/* consumes the current char if it was c 
   and returns true, else returns false */
#define match_char(c) c == *l->current ? (l->current++, 1) : 0

/* skips the short comments */
static void line_comment(Lexer l) {
    while (!at_end() && peek_char() != '\n')
        cons_char();
    
    /* not to consume the newline at the end 
       of the comment as a token*/
    if (peek_char() == '\n') {
        l->line++;
        cons_char();
    }
}

/* skips the long comments */
static void long_comment(Lexer l) {    
    cons_char(); /* '#' */
    cons_char(); /* '-' */

    int unclosed = 1; /* number of unclosed '#-' */

    while (unclosed) {
        while (!at_end() && ((peek_char() != '-') ||
                             (peek_next() != '#'))) {
            cons_char();

            /* '#-' found */
            if ((peek_char() == '#') && (peek_next() == '-'))
                unclosed++;
        
            if (peek_char() == '\n')
                l->line++;
        }

        /* '-#' found */
        if (!at_end()) {
            unclosed--;
            cons_char(); /* '-' */
            cons_char(); /* '#' */
        } else {
            return; /* unterminated long comment is not an error */
        }
    }

    if (peek_char() == '-') {
        cons_char(); /* '-' */
        cons_char(); /* '#' */
    }
}

static void skip_whitespace(Lexer l) {    
    for (;;) {
        switch (peek_char()) {
        case ' ':
        case '\t':
        case '\r':
            cons_char();
            break;
        case '#':
            if (peek_next() == '-')
                long_comment(l);
            else
                line_comment(l);
            break;
        default:
            return;
        }
    }           
}

/* match the rest of the keyword with the rest of the token */
#define match_keyword(rest)                                 \
    (sizeof(rest) - 1 == l->current - l->fixed - 1 &&       \
     !strncmp(rest, l->fixed + 1, sizeof(rest) - 1))

/* consumes a keywords if matched, otherwise an identifiers */
static Token cons_ident(Lexer l) {
    /* extract the whole token first */
    char start_ch = prev_char();
    
    while (!at_end() && (isalnum(peek_char()) || peek_char() == '_'))
        cons_char();

    /* probably faster than hash table but need to be tested though */
    switch (start_ch) {
    case 'a':
        if (match_keyword("nd"))
            return valid_token(l, TK_AND);
        break;
        
    case 'b':
        if (match_keyword("reak"))
            return valid_token(l, TK_BREAK);
        break;
        
    case 'c':
        if (match_keyword("ase"))
            return valid_token(l, TK_CASE);
        else if (match_keyword("ontinue"))
            return valid_token(l, TK_CONTINUE);
        break;
        
    case 'd':
        if (match_keyword("o"))
            return valid_token(l, TK_DO);
        break;
        
    case 'e':
        if (match_keyword("lif"))
            return valid_token(l, TK_ELIF);
        else if (match_keyword("lse"))
            return valid_token(l, TK_ELSE);
        else if (match_keyword("nd"))
            return valid_token(l, TK_END);
        break;
        
    case 'f':
        if (match_keyword("alse"))
            return valid_token(l, TK_FALSE);
        else if (match_keyword("n"))
            return valid_token(l, TK_FN);
        else if (match_keyword("or"))
            return valid_token(l, TK_FOR);
        break;
        
    case 'i':
        if (match_keyword("f"))
            return valid_token(l, TK_IF);
        else if (match_keyword("n"))
            return valid_token(l, TK_IN);
        break;
        
    case 'l':
        if (match_keyword("et"))
            return valid_token(l, TK_LET);
        break;
        
    case 'm':
        if (match_keyword("atch"))
            return valid_token(l, TK_MATCH);
        break;
        
    case 'n':
        if (match_keyword("il"))
            return valid_token(l, TK_NIL);
        else if (match_keyword("ot"))
            return valid_token(l, TK_NOT);
        break;
        
    case 'o':
        if (match_keyword("r"))
            return valid_token(l, TK_OR);
        break;
        
    case 'r':
        if (match_keyword("eturn"))
            return valid_token(l, TK_RETURN);
        break;
        
    case 't':
        if (match_keyword("rue"))
            return valid_token(l, TK_TRUE);
        break;
        
    case 'w':
        if (match_keyword("hile"))
            return valid_token(l, TK_WHILE);
        break;        
    }

    return valid_token(l, TK_IDENT);  
}

#define is_hexa_digit(n)                        \
    (n >= '0' && n <= '9') ||                   \
    (n >= 'a' && n <= 'f') ||                   \
    (n >= 'A' && n <= 'F')

#define is_octal_digit(n)                       \
    (n >= '0' && n <= '7')

#define is_bin_digit(n)                         \
    (n == '0' || n == '1')

/* consumes number types */
Token cons_num(Lexer l) {
    
    char start_ch = prev_char();
    int is_float = 0;

    /* case of hexa, octal and binary numbers */
    if (start_ch == '0') {
        switch (peek_char()) {
        case 'x':
        case 'X':
            cons_char();
            while (is_hexa_digit(peek_char())) cons_char();
            return valid_token(l, TK_INT);
        case 'o':
        case 'O':
            cons_char();
            while (is_octal_digit(peek_char())) cons_char();
            return valid_token(l, TK_INT);
        case 'b':
        case 'B':
            cons_char();
            while(is_bin_digit(peek_char())) cons_char();
            return valid_token(l, TK_INT);
        }
    }
    
    while (!at_end() && isdigit(peek_char()))
        cons_char();

    /* regular decimal point number */
    if (peek_char() == '.' && isdigit(peek_next())) {
        is_float = 1;
        cons_char();
        while (!at_end() && isdigit(peek_char()))
            cons_char();
    }

    /* scientific notation */
    if ((peek_char() == 'e' || peek_char() == 'E')
        && (isdigit(peek_next())
            || (peek_next() == '+')
            || (peek_next() == '-'))) {
        is_float = 1;
        
        cons_char();  /* consume 'e/E' */
        cons_char();  /* consume '+/-/<digit> */
        
        /* malformed scientific notation (ex. '1e+') */
        if (!isdigit(prev_char()) && !isdigit(peek_char()))
            return error_token(l, MSG_MALSC);
        
        while (!at_end() && isdigit(peek_char()))
            cons_char();
    }

    return valid_token(l, is_float ? TK_FLOAT : TK_INT);
}

/* consumes escaped strings */
static Token cons_str(Lexer l) {    
    char start_ch = prev_char();
    char *str_start = (char*)((l->current)-1);

    /* calculate the string size making sure that the string
       doesn't terminate on an escaped single or double quotes */
    while (!at_end() &&
           peek_char() != start_ch && peek_char() != '\n') {
        if (peek_char() == '\\') {
            cons_char();
            if (peek_char() == '\n')
                break;
            cons_char();
        } else
            cons_char();
    }
    
    if (at_end() || peek_char() == '\n')
        return error_token(l, MSG_UNTER);

    cons_char();   /* consume the final quote */
    
    char *str_end = (char*)l->current;
    int size = str_end - str_start;

    /* check for escaping erros */
    char *buf = alloc(size, R_SECN);  /* escape string buffer */
    char *res = escape(str_start, buf, size);

    /* escape error */
    if (res != NULL) {
        Token err = error_token(l, MSG_INVLD);
        err->lexeme = res;  /* the lexeme point to the escape error */
        err->length = l->current - res;
        return err;
    }
    
    /* note that the used lexeme is the original unescaped
       string, the actual escaping happen in the runtime. */
    return valid_token(l, TK_STR);
}

/* consume raw strings without any escaping */
Token cons_rstr(Lexer l) {
    char start_ch = prev_char();

    while (!at_end() && peek_char() != start_ch) {
        if (peek_char() == '\n') l->line++;
        cons_char();
    }

    /* the final qoute */
    cons_char();
    
    if (at_end())
        return error_token(l, MSG_UNTER);

    return valid_token(l, TK_RSTR);
}

/* consume the current token, then return it. */
static Token consume(Lexer l) {    
    /* ignore any whitespace characters */
    skip_whitespace(l);

    l->fixed = l->current;
    char c = cons_char();
     
    if (isalpha(c) || c == '_') return cons_ident(l);
    if (isdigit(c)) return cons_num(l);

    switch(c) {      
    /* one character tokens */
    case '@':
        return valid_token(l, TK_AT);
    case '|':
        return valid_token(l, TK_PIPE);
    case '+':
        return valid_token(l, TK_PLUS);
    case '*':
        return valid_token(l, TK_ASTERISK);
    case '/':
        return valid_token(l, TK_SLASH);
    case '%':
        return valid_token(l, TK_PERCENT);
    case '(':
        return valid_token(l, TK_LPAREN);
    case ')':
        return valid_token(l, TK_RPAREN);
    case '{':
        return valid_token(l, TK_LBRACE);
    case '}':
        return valid_token(l, TK_RBRACE);
    case '[':
        return valid_token(l, TK_LBRACKET);
    case ']':
        return valid_token(l, TK_RBRACKET);
    case ',':
        return valid_token(l, TK_COMMA);
    case '.':
        return valid_token(l, TK_DOT);
    case ':':
        return valid_token(l, TK_COLON);
    case ';':
        return valid_token(l, TK_SEMICOLON);
        
    /* possible two character tokens */
    case '<':
        if (match_char('='))
            return valid_token(l, TK_LT_EQ);
        return valid_token(l, TK_LT);
        
    case '>':
        if (match_char('='))
            return valid_token(l, TK_GT_EQ);
        return valid_token(l, TK_GT);

    case '=':
        if (match_char('='))
            return valid_token(l, TK_EQ_EQ);
        return valid_token(l, TK_EQ);

    case '-':
        if (match_char('>'))
            return valid_token(l, TK_DASH_GT);
        return valid_token(l, TK_MINUS);

    case '!':
        if (match_char('='))
            return valid_token(l, TK_BANG_EQ);


    case '"':
    case '\'':
        return cons_str(l);

    case'`':
        return cons_rstr(l);
            
    case '\n': {
        Token tok = valid_token(l, TK_NL);
        tok->lexeme = "";
        tok->length = 0;

        l->line++;
        return tok;
    }
        
    case '\0':
    case EOF:
        return valid_token(l, TK_EOF);
    }
    return error_token(l, MSG_UNREC);
}

/** INTERFACE **/

char *scan_file(const char *file) {
    FILE *f = fopen(file, "rb");
    if (f == NULL)
        return NULL;
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f)+1;
    char *buff = alloc(size, R_PERM);

    fseek(f, 0, SEEK_SET);
    fread(buff, 1, size, f);
    fclose(f);

    return buff;
}

void init_lexer(Lexer l, const char *src, const char *file) {
    l->current = src;
    l->fixed = src;
    l->file = str(file);
    l->line = 1;
    l->been_error = 0;
    l->tokens = List_new(l->reg);
    l->errors = List_new(l->reg);
}

Lexer lexer_new(const char *src, const char *file, Region_N reg) {
    Lexer l = make(l, reg);
    l->reg = reg;
    init_lexer(l, src, file);

    return l;
}

Token cons_token(Lexer l) {
    return consume(l);
}

List cons_tokens(Lexer l) {
    Token tok = cons_token(l);

    while (tok->type != TK_EOF) {
        if (tok->type == TK_ERR) {
            l->been_error = 1;
            List_append(l->errors, tok);
        } else {
            List_append(l->tokens, tok);
        }
        tok = cons_token(l);
    }

    List_append(l->tokens, tok);  /* EOF token */
    return l->tokens; 
}

int lexer_been_error(Lexer l) {
    return l->been_error;
}

List lexer_errors(Lexer l) {
    return l->been_error ? l->errors : NULL;
}
