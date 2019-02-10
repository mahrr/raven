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


char *scan_file(const char *file) {
    FILE *f = fopen(file, "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f)+1;
    char *buff = alloc(size, R_PERM);

    fseek(f, 0, SEEK_SET);
    fread(buff, 1, size, f);
    fclose(f);

    return buff;
}

void init_lexer(lexer *l, char *src, const char *file) {
    assert(l != NULL);
    l->current = src;
    l->fixed = src;
    l->file_name = str(file);
    l->line = 1;
}

/* initializes a new token with a lexeme */
#define new_token(t)                                    \
    (token){t, strn(l->fixed, l->current - l->fixed),   \
            l->file_name, l->line}                      \

/* initializes a new token without a lexeme */
#define new_ntoken(t) (token) {t, NULL, l->file_name, l->line} 

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
        while (!at_end() && (peek_char() != '-') || (peek_next() != '#')) {
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

/* consumes keywords if matched, or identifiers */
token cons_ident(lexer *l) {
    /* extract the whole token first */
    char start_ch = prev_char();
    
    while (!at_end() && isalnum(peek_char()) || peek_char() == '_')
        cons_char();

    /* @@ probably faster than hash table but need to be tested though */
    switch (start_ch) {
    case 'a':
        if (match_keyword("nd"))
            return new_ntoken(TK_AND);
        break;
        
    case 'b':
        if (match_keyword("reak"))
            return new_ntoken(TK_BREAK);
        break;
        
    case 'c':
        if (match_keyword("ase"))
            return new_ntoken(TK_CASE);
        else if (match_keyword("ontinue"))
            return new_ntoken(TK_CONTINUE);
        break;
        
    case 'd':
        if (match_keyword("o"))
            return new_ntoken(TK_DO);
        break;
        
    case 'e':
        if (match_keyword("lif"))
            return new_ntoken(TK_ELIF);
        else if (match_keyword("lse"))
            return new_ntoken(TK_ELSE);
        else if (match_keyword("nd"))
            return new_ntoken(TK_END);
        break;
        
    case 'f':
        if (match_keyword("alse"))
            return new_ntoken(TK_FALSE);
        else if (match_keyword("n"))
            return new_ntoken(TK_FN);
        else if (match_keyword("or"))
            return new_ntoken(TK_FOR);
        break;
        
    case 'i':
        if (match_keyword("f"))
            return new_ntoken(TK_IF);
        break;
        
    case 'l':
        if (match_keyword("et"))
            return new_ntoken(TK_LET);
        break;
        
    case 'm':
        if (match_keyword("atch"))
            return new_ntoken(TK_MATCH);
        break;
        
    case 'n':
        if (match_keyword("il"))
            return new_ntoken(TK_NIL);
        else if (match_keyword("ot"))
            return new_ntoken(TK_NOT);
        break;
        
    case 'o':
        if (match_keyword("r"))
            return new_ntoken(TK_OR);
        break;
        
    case 'r':
        if (match_keyword("eturn"))
            return new_ntoken(TK_RETURN);
        break;
        
    case 't':
        if (match_keyword("rue"))
            return new_ntoken(TK_TRUE);
        break;
        
    case 'w':
        if (match_keyword("hile"))
            return new_ntoken(TK_WHILE);
        break;
        
    }
    return new_token(TK_IDENT);  
}

#define is_hexa_digit(n)                        \
    (n >= '0' && n <= '9') ||                   \
    (n >= 'a' && n <= 'e') ||                   \
    (n >= 'A' && n <= 'E')

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
            return new_token(TK_INVALID_SCIEN);
        
        while (!at_end() && isdigit(peek_char()))
            cons_char();
    }

    return new_token(is_float ? TK_FLOAT : TK_INT);
}

/* consumes escaped strings */
token cons_str(lexer *l) {    
    char start_ch = prev_char();
    char *str_start = l->current;

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
    char *str_end = l->current;
    
    if (at_end() || peek_char() == '\n')
        return new_token(TK_UNTERMIN_STR);

    cons_char();   /* consume the final qoute */

    /* the state of escapeing :
         without any errors             -> 0x00
         invalid escape                 -> 0x01
         octal out of range escape      -> 0x02
         octal missing digits           -> 0x04
         octal invalid digits           -> 0x08
         hexa missing digits            -> 0x16
         hexa invalid digits            -> 0x32
    */
    int state;
    int size = str_end - str_start;
    char *escaped_str = alloc(size, R_PERM);
    state = escape(str_start, escaped_str, size);

    switch (state) {  
    case INV_ESCP:
        return new_token(TK_INVALID_ESCP);
    case OCT_OFRG:
        return new_token(TK_OCT_OUTOFR_ESCP);
    case OCT_MISS:
        return new_token(TK_OCT_MISS_ESCP);
    case OCT_INVL:
        return new_token(TK_OCT_INVL_ESCP);
    case HEX_MISS:
        return new_token(TK_HEX_MISS_ESCP);
    case HEX_INVL:
        return new_token(TK_HEX_INVL_ESCP);
    default:
        return (token){TK_STR, escaped_str, l->file_name, l->line};
    }
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
        return new_token(TK_UNTERMIN_STR);
    
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
        return new_ntoken(TK_AT);
    case ':':
        return new_ntoken(TK_COLON);
    case '+':
        return new_ntoken(TK_PLUS);
    case '*':
        return new_ntoken(TK_ASTERISK);
    case '/':
        return new_ntoken(TK_SLASH);
    case '%':
        return new_ntoken(TK_PERCENT);
    case '|':
        return new_ntoken(TK_PIPE);
    case '&':
        return new_ntoken(TK_AMPERSAND);
    case '^':
        return new_ntoken(TK_CARET);
    case '~':
        return new_ntoken(TK_TILDE);
    case '(':
        return new_ntoken(TK_LPAREN);
    case ')':
        return new_ntoken(TK_RPAREN);
    case '{':
        return new_ntoken(TK_LBRACE);
    case '}':
        return new_ntoken(TK_RBRACE);
    case '[':
        return new_ntoken(TK_LBRACKET);
    case ']':
        return new_ntoken(TK_RBRACKET);
    case ',':
        return new_ntoken(TK_COMMA);
        
    /* possible two character tokens */
    case '<':
        if (match_char('<'))
            return new_ntoken(TK_LT_LT);
        else if (match_char('='))
            return new_ntoken(TK_LT_EQ);
        return new_ntoken(TK_LT);
        
    case '>':
        if (match_char('>'))
            return new_ntoken(TK_GT_GT);
        else if (match_char('='))
            return new_ntoken(TK_GT_EQ);
        return new_ntoken(TK_GT);

    case '=':
        if (match_char('='))
            return new_ntoken(TK_EQ_EQ);
        return new_ntoken(TK_EQ);

    case '-':
        if (match_char('>'))
            return new_ntoken(TK_DASH_GT);
        return new_ntoken(TK_MINUS);

    case '!':
        if (match_char('='))
            return new_ntoken(TK_BANG_EQ);
        return new_token(TK_UNRECOG);

    case '"':
    case '\'':
        return cons_str(l);

    case'`':
        return cons_rstr(l);
            
    case '\n':
        l->line++;
        /* It's really wired to register the line of newline token,
           I will stick with the more technical solution and
           register it in the line '\n' character was found */
        return (token){TK_NL, NULL, l->file_name, l->line-1};
    case '\0':
    case EOF:
        return new_ntoken(TK_EOF);

    default:
        return new_token(TK_UNRECOG);
    }       
}

extern token *alloc_token(token t, unsigned reg) {
    token *tokp = make(tokp, reg);
    tokp->type = t.type;
    tokp->lexeme = t.lexeme;
    tokp->file = t.file;
    tokp->line = t.line;

    return tokp;
}

extern token_list *cons_tokens(lexer *l) {
    list *tokens = NULL;
    list *error_tokens = NULL;
    int been_error = 0;
    
    token curr_tok = cons_token(l);
    while (curr_tok.type != TK_EOF) {
        token *tp = alloc_token(curr_tok, R_FIRS);
        if (is_errtok(tp)) {
            been_error = 1;
            error_tokens = append_list(error_tokens, tp);
        }
        tokens = append_list(tokens, tp);
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
    "TK_DOT", "TK_AT",
    
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
    "TK_COLON", "TK_EQ", "TK_IN",
    "TK_NL", 
    
    /* Errors and end of file */
    "TK_UNRECOG", "TK_UNTERMIN_STR",
    "TK_INVALID_ESCP", "TK_OCT_OUTOFR_ESCP",
    "TK_OCT_MISS_ESCP", "TK_OCT_INVL_ESCP",
    "TK_HEX_MISS_ESCP", "TK_HEX_INVL_ESCP",
    "TK_INVALID_SCIEN",
    
    "TK_EOF",
};

void print_token(token *t) {
    printf("[%s @line %ld] :: %s :: %s\n",
           t->file,
           t->line,
           t->lexeme == NULL ? " " : t->lexeme,
           tok_types_str[t->type]);
}

