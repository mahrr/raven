/*
 * (lexer.c | 22 Nov 18 | Ahmad Maher)
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "lexer.h"
#include "alloc.h"
#include "salloc.h"
#include "strutil.h"
#include "list.h"

/* hardcoded lexical errors messages */
#define msg_UNREC "unrecognize syntax"
#define msg_UNTER "unterminated string"
#define msg_INVLD "invalid escape sequence"
#define msg_MALSC "malformed scientific notation"

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

void init_lexer(lexer *l, char *src, const char *file) {
    l->current = src;
    l->fixed = src;
    l->file_name = str(file);
    l->line = 1;
}

/* initializes a new token with a lexeme */
#define new_token(t)                                   \
    (token){t, l->fixed, l->current - l->fixed,        \
            l->file_name, l->line, NULL}

/* initializes a new token without a lexeme */
#define empty_token(t)                                   \
    (token) {t, NULL, 0, l->file_name, l->line, NULL}

/* initialize an error token */
#define error_token(msg)                                \
    (token) {TK_ERR, l->fixed, l->current - l->fixed,   \
            l->file_name, l->line, str(msg)}

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
#define match_char(c) c == *l->current ? (l->current++, true) : false

/* skips the short comments */
static void line_comment(lexer *l) {
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
static void long_comment(lexer *l) {    
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

static void skip_whitespace(lexer *l) {    
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
static token cons_ident(lexer *l) {
    /* extract the whole token first */
    char start_ch = prev_char();
    
    while (!at_end() && (isalnum(peek_char()) || peek_char() == '_'))
        cons_char();

    /* @@ probably faster than hash table but need to be tested though */
    switch (start_ch) {
    case 'a':
        if (match_keyword("nd"))
            return empty_token(TK_AND);
        break;
        
    case 'b':
        if (match_keyword("reak"))
            return empty_token(TK_BREAK);
        break;
        
    case 'c':
        if (match_keyword("ase"))
            return empty_token(TK_CASE);
        else if (match_keyword("ontinue"))
            return empty_token(TK_CONTINUE);
        break;
        
    case 'd':
        if (match_keyword("o"))
            return empty_token(TK_DO);
        break;
        
    case 'e':
        if (match_keyword("lif"))
            return empty_token(TK_ELIF);
        else if (match_keyword("lse"))
            return empty_token(TK_ELSE);
        else if (match_keyword("nd"))
            return empty_token(TK_END);
        break;
        
    case 'f':
        if (match_keyword("alse"))
            return empty_token(TK_FALSE);
        else if (match_keyword("n"))
            return empty_token(TK_FN);
        else if (match_keyword("or"))
            return empty_token(TK_FOR);
        break;
        
    case 'i':
        if (match_keyword("f"))
            return empty_token(TK_IF);
        else if (match_keyword("n"))
            return empty_token(TK_IN);
        break;
        
    case 'l':
        if (match_keyword("et"))
            return empty_token(TK_LET);
        break;
        
    case 'm':
        if (match_keyword("atch"))
            return empty_token(TK_MATCH);
        break;
        
    case 'n':
        if (match_keyword("il"))
            return empty_token(TK_NIL);
        else if (match_keyword("ot"))
            return empty_token(TK_NOT);
        break;
        
    case 'o':
        if (match_keyword("r"))
            return empty_token(TK_OR);
        break;
        
    case 'r':
        if (match_keyword("eturn"))
            return empty_token(TK_RETURN);
        break;
        
    case 't':
        if (match_keyword("rue"))
            return empty_token(TK_TRUE);
        break;
        
    case 'w':
        if (match_keyword("hile"))
            return empty_token(TK_WHILE);
        break;        
    }

    return new_token(TK_IDENT);  
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
token cons_num(lexer *l) {
    
    char start_ch = prev_char();
    bool is_float = false;

    /* case of hexa, octal and binary numbers */
    if (start_ch == '0') {
        switch (peek_char()) {
        case 'x':
        case 'X':
            cons_char();
            while (is_hexa_digit(peek_char())) cons_char();
            return new_token(TK_INT);
        case 'o':
        case 'O':
            cons_char();
            while (is_octal_digit(peek_char())) cons_char();
            return new_token(TK_INT);
        case 'b':
        case 'B':
            cons_char();
            while(is_bin_digit(peek_char())) cons_char();
            return new_token(TK_INT);
        }
    }
    
    while (!at_end() && isdigit(peek_char()))
        cons_char();

    /* regular decimal point number */
    if (peek_char() == '.' && isdigit(peek_next())) {
        is_float = true;
        cons_char();
        while (!at_end() && isdigit(peek_char()))
            cons_char();
    }

    /* scientific notation */
    if ((peek_char() == 'e' || peek_char() == 'E')
        && (isdigit(peek_next())
            || (peek_next() == '+')
            || (peek_next() == '-'))) {
        is_float = true;
        
        cons_char();  /* consume 'e/E' */
        cons_char();  /* consume '+/-/<digit> */
        
        /* malformed scientific notation (ex. '1e+') */
        if (!isdigit(prev_char()) && !isdigit(peek_char()))
            return error_token(msg_MALSC);
        
        while (!at_end() && isdigit(peek_char()))
            cons_char();
    }

    return new_token(is_float ? TK_FLOAT : TK_INT);
}

/* consumes escaped strings */
static token cons_str(lexer *l) {    
    char start_ch = prev_char();
    char *str_start = (l->current)-1;

    /* calculate the string size and sure that the string 
       doesn't terminate in an escaped on single or double qoute */
    while (!at_end() && peek_char() != start_ch && peek_char() != '\n') {
        if (peek_char() == '\\') {
            cons_char();
            if (peek_char() == '\n')
                break;
            cons_char();
        } else
            cons_char();
    }
    
    if (at_end() || peek_char() == '\n')
        return error_token(msg_UNTER);

    cons_char();   /* consume the final quote */
    
    char *str_end = l->current;
    int size = str_end - str_start;

    /* allocate *es in R_SECN because if the escaping,
       failed we can claim the memory back the next phase. */
    char *es = alloc(size, R_SECN);
    char *s = escape(str_start, es, size);

    /* escape error */
    if (s != NULL) 
        return (token){TK_ERR, s, l->current - s, l->file_name,
                l->line, msg_INVLD};
    
    /* copy *es because *es is allocated in R_SECN. */
    char *escaped = str(es);
    
    return (token){TK_STR, escaped, strlen(escaped),
            l->file_name, l->line, NULL};
}

/* consume raw strings without any escaping */
token cons_rstr(lexer *l) {
    char start_ch = prev_char();

    while (!at_end() && peek_char() != start_ch) {
        if (peek_char() == '\n') l->line++;
        cons_char();
    }

    /* the final qoute */
    cons_char();
    
    if (at_end())
        return error_token(msg_UNTER);

    return new_token(TK_RSTR);
}

extern token cons_token(lexer *l) {    
    /* ignore any whitespace characters */
    skip_whitespace(l);

    l->fixed = l->current;
    char c = cons_char();
     
    if (isalpha(c) || c == '_') return cons_ident(l);
    if (isdigit(c)) return cons_num(l);

    switch(c) {      
    /* one character tokens */
    case '@':
        return empty_token(TK_AT);
    case '+':
        return empty_token(TK_PLUS);
    case '*':
        return empty_token(TK_ASTERISK);
    case '/':
        return empty_token(TK_SLASH);
    case '%':
        return empty_token(TK_PERCENT);
    case '|':
        return empty_token(TK_PIPE);
    case '&':
        return empty_token(TK_AMPERSAND);
    case '^':
        return empty_token(TK_CARET);
    case '~':
        return empty_token(TK_TILDE);
    case '(':
        return empty_token(TK_LPAREN);
    case ')':
        return empty_token(TK_RPAREN);
    case '{':
        return empty_token(TK_LBRACE);
    case '}':
        return empty_token(TK_RBRACE);
    case '[':
        return empty_token(TK_LBRACKET);
    case ']':
        return empty_token(TK_RBRACKET);
    case ',':
        return empty_token(TK_COMMA);
    case ';':
        return empty_token(TK_SEMICOLON);
        
    /* possible two character tokens */
    case '<':
        if (match_char('<'))
            return empty_token(TK_LT_LT);
        else if (match_char('='))
            return empty_token(TK_LT_EQ);
        return empty_token(TK_LT);
        
    case '>':
        if (match_char('>'))
            return empty_token(TK_GT_GT);
        else if (match_char('='))
            return empty_token(TK_GT_EQ);
        return empty_token(TK_GT);

    case '=':
        if (match_char('='))
            return empty_token(TK_EQ_EQ);
        return empty_token(TK_EQ);

    case '-':
        if (match_char('>'))
            return empty_token(TK_DASH_GT);
        return empty_token(TK_MINUS);

    case ':':
        if (match_char(':'))
            return empty_token(TK_COL_COL);
        return empty_token(TK_COLON);

    case '!':
        if (match_char('='))
            return empty_token(TK_BANG_EQ);


    case '"':
    case '\'':
        return cons_str(l);

    case'`':
        return cons_rstr(l);
            
    case '\n':
        l->line++;
        /* It's really weird to register the line of newline token,
           I will stick with the more technical solution and
           register it in the line '\n' character was found */
        return (token){TK_NL, NULL, 0, l->file_name, l->line-1, NULL};
    case '\0':
    case EOF:
        return empty_token(TK_EOF);
    }
    return error_token(msg_UNREC);
}

extern token *alloc_token(token t, unsigned reg) {
    token *tokp = make(tokp, reg);
    tokp->type = t.type;
    tokp->lexeme = t.lexeme;
    tokp->length = t.length;
    tokp->file = t.file;
    tokp->line = t.line;
    tokp->err_msg = t.err_msg;

    return tokp;
}

extern token_list *cons_tokens(lexer *l) {
    list *tokens = NULL;
    list *error_tokens = NULL;
    int been_error = 0;
    
    token curr_tok = cons_token(l);
    while (curr_tok.type != TK_EOF) {
        token *tp = alloc_token(curr_tok, R_FIRS);
        if (tp->type == TK_ERR) {
            been_error = 1;
            error_tokens = append_list(error_tokens, tp);
        } else {
            tokens = append_list(tokens, tp);
        }
        curr_tok = cons_token(l);
    }

    token_list *tl = make(tl, R_FIRS);
    tl->tokens = tokens;
    tl->been_error = been_error;
    tl->error_tokens = error_tokens;
    return tl; 
}

/* debugging stuff */
char *tok_types_str[] = {
    /* Literals */
    "TK_INT", "TK_FLOAT", "TK_STR",
    "TK_RSTR", "TK_FALSE", "TK_TRUE",
    "TK_NIL",
    
    /* Keywords */
    "TK_FN", "TK_RETURN", "TK_LET", "TK_DO",
    "TK_END", "TK_IF", "TK_ELIF", "TK_ELSE",
    "TK_FOR", "TK_WHILE", "TK_CONTINUE",
    "TK_BREAK", "TK_MATCH", "TK_CASE",
    
    /* Identefier */
    "TK_IDENT",
    
    /* Operators */
    "TK_AND", "TK_OR", "TK_NOT",
    "TK_DOT", "TK_AT", "TK_COL_COL",
    
    /* Arthimetik Operators */
    "TK_PLUS", "TK_MINUS", "TK_ASTERISK",
    "TK_SLASH", "TK_PERCENT",
    
    /* Ordering Operators */
    "TK_LT", "TK_GT", "TK_EQ_EQ",
    "TK_BANG_EQ", "TK_LT_EQ",
    "TK_GT_EQ",
    
    /* Logic Operators */
    "TK_PIPE", "TK_AMPERSAND", "TK_CARET",
    "TK_TILDE", "TK_GT_GT", "TK_LT_LT",
    
    /* Delimiters */
    "TK_LPAREN", "TK_RPAREN",
    "TK_LBRACE", "TK_RBRACE",
    "TK_LBRACKET", "TK_RBRACKET",
    "TK_COMMA", "TK_DASH_GT",
    "TK_COLON", "TK_SEMICOLON",
    "TK_EQ", "TK_IN",
    "TK_NL", 
    
    /* Errors and end of file */
    "TK_ERR", "TK_EOF",
};

void print_token(token *t) {
    printf("[%s @line %ld] :: %.*s (%d) :: %s\n",
           t->file,
           t->line,
           t->length,
           t->lexeme == NULL ? " " : t->lexeme,
           t->length,
           tok_types_str[t->type]);
}

