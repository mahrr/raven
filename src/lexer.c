/*
 * (lexer.c | 22 Nov 18 | Ahmad Maher)
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

/* initializes a new token */
#define new_token(t)                                    \
    (token){t, strn(l->fixed, l->current - l->fixed),   \
            l->file_name, l->line}                      \

/* checks if the lexer reached the end of the source */
#define at_end() *l->current == '\0'

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
#define line_comment() \
    while (!at_end() && peek_char() != '\n') cons_char()

/* skips the long comment. If it is an unterminated 
   comment, it yeilds a token_error */
#define long_comment()                                  \
    while (!at_end() &&                                 \
           peek_char() != '*' &&                        \
           peek_next() != '/') {                        \
        if (peek_char() == '\n') line++;                \
        cons_char();                                    \
    }                                                   \
    if (at_end())                                       \
        return new_token(UNTERMIN_COMM)                 \
    cons_char(); cons_char()

static void skip_whitespace(lexer *l) {    
    for (;;) {
        switch (peek_char()) {
        case ' ':
        case '\t':
        case '\r':
            cons_char();
            break;
        case '\n':
            l->line++;
            cons_char();
            break;
        case '/':
            if (match_char('/'))
                line_comment();
            else
                return;
            break;
        default:
            return;
        }
    }           
}

/* match the rest of the keyword with the rest of the token */
#define match_keyword(rest)                                 \
    (!at_end() && !strncmp(rest, l->fixed+1, l->current - l->fixed-1))

/*
bool match_keyword(const char *rest) {
    if (!at_end() && !strncmp(rest, fixed+1, current-fixed-1))
        return true;
    return false;
}
*/

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
            return new_token(AND);
        break;
        
    case 'b':
        if (match_keyword("ool"))
            return new_token(BOOL_T);
        else if (match_keyword("reak"))
            return new_token(BREAK);
        break;
        
    case 'c':
        if (match_keyword("ase"))
            return new_token(CASE);
        else if (match_keyword("ontinue"))
            return new_token(CONTINUE);
        break;
        
    case 'd':
        if (match_keyword("o"))
            return new_token(DO);
        break;
        
    case 'e':
        if (match_keyword("lif"))
            return new_token(ELIF);
        else if (match_keyword("lse"))
            return new_token(ELSE);
        else if (match_keyword("nd"))
            return new_token(END);
        break;
        
    case 'f':
        if (match_keyword("alse"))
            return new_token(FALSE);
        else if (match_keyword("in"))
            return new_token(FIN);
        else if (match_keyword("loat"))
            return new_token(FLOAT_T);
        else if (match_keyword("n"))
            return new_token(FN);
        else if (match_keyword("or"))
            return new_token(FOR);
        break;
        
    case 'h':
        if (match_keyword("andle"))
            return new_token(HANDLE);
        break;
        
    case 'i':
        if (match_keyword("f"))
            return new_token(IF);
        else if (match_keyword("nt"))
            return new_token(INT_T);
        break;
        
    case 'l':
        if (match_keyword("et"))
            return new_token(LET);
        else if (match_keyword("ist"))
            return new_token(LIST_T);
        break;
        
    case 'm':
        if (match_keyword("atch"))
            return new_token(MATCH);
        else if (match_keyword("odule"))
            return new_token(MODULE);
        break;
        
    case 'n':
        if (match_keyword("il"))
            return new_token(NIL);
        else if (match_keyword("ot"))
            return new_token(NOT);
        else if (match_keyword("um"))
            return new_token(NUM_T);
        break;
        
    case 'o':
        if (match_keyword("r"))
            return new_token(OR);
        else if (match_keyword("f"))
            return new_token(OF);
        break;
        
    case 'r':
        if (match_keyword("aise"))
            return new_token(RAISE);
        else if (match_keyword("eturn"))
            return new_token(RETURN);
        break;
        
    case 's':
        if (match_keyword("tr"))
            return new_token(STR_T);
        break;
        
    case 't':
        if (match_keyword("hen"))
            return new_token(THEN);
        else if (match_keyword("rue"))
            return new_token(TRUE);
        else if (match_keyword("ype"))
            return new_token(TYPE);
        break;
        
    case 'u':
        if (match_keyword("se"))
            return new_token(USE);
        break;
        
    case 'w':
        if (match_keyword("hile"))
            return new_token(WHILE);
        break;
        
    }
    return new_token(IDENT);  
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
            return new_token(INT);
        case 'o':
        case 'O':
            cons_char();
            while (is_octal_digit(peek_char())) cons_char();
            return new_token(INT);
        case 'b':
        case 'B':
            cons_char();
            while(is_bin_digit(peek_char())) cons_char();
            return new_token(INT);
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
            return new_token(INVALID_SCIEN);
        
        while (!at_end() && isdigit(peek_char()))
            cons_char();
    }

    return new_token(is_float ? FLOAT : INT);
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
        return new_token(UNTERMIN_STR);

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
        return new_token(INVALID_ESCP);
    case OCT_OFRG:
        return new_token(OCT_OUTOFR_ESCP);
    case OCT_MISS:
        return new_token(OCT_MISS_ESCP);
    case OCT_INVL:
        return new_token(OCT_INVL_ESCP);
    case HEX_MISS:
        return new_token(HEX_MISS_ESCP);
    case HEX_INVL:
        return new_token(HEX_INVL_ESCP);
    default:
        return (token){STRING, escaped_str, l->file_name, l->line};
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
        return new_token(UNTERMIN_RSTR);
    
    return new_token(R_STRING);
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
    case '#':
        return new_token(HASH);
    case '@':
        return new_token(AT);
    case ':':
        return new_token(COLON);
    case '+':
        return new_token(PLUS);
    case '-':
        return new_token(MINUS);
    case '*':
        return new_token(ASTERISK);
    case '/':
        return new_token(SLASH);
    case '%':
        return new_token(PERCENT);
    case '|':
        return new_token(PIPE);
    case '&':
        return new_token(AMPERSAND);
    case '^':
        return new_token(CARET);
    case '~':
        return new_token(TILDE);
    case '(':
        return new_token(LPAREN);
    case ')':
        return new_token(RPAREN);
    case '{':
        return new_token(LBRACE);
    case '}':
        return new_token(RBRACE);
    case '[':
        return new_token(LBRACKET);
    case ']':
        return new_token(RBRACKET);
    case ',':
        return new_token(COMMA);
        
    /* possible two character tokens */
    case '.':
        if (match_char('.'))
            return new_token(DOT_DOT);
        return new_token(DOT);
        
    case '<':
        if (match_char('<'))
            return new_token(LESS_LESS);
        else if (match_char('='))
            return new_token(LESS_EQUAL);
        return new_token(LESS);
        
    case '>':
        if (match_char('>'))
            return new_token(GREAT_GREAT);
        else if (match_char('='))
            return new_token(GREAT_EQUAL);
        else if (match_char('|'))
            return new_token(GREAT_PIPE);
        return new_token(GREAT);

    case '=':
        if (match_char('='))
            return new_token(EQUAL_EQUAL);
        else if (match_char('>'))
            return new_token(EQUAL_GREAT);
        return new_token(EQUAL);

    case '!':
        if (match_char('='))
            return new_token(BANG_EQUAL);
        else if (match_char('"') ||
                 match_char('\''))
            return cons_rstr(l);
        return new_token(UNRECOG);

    case '"':
    case '\'':
        return cons_str(l);
    case '\0':
    case EOF:
        return new_token(EOF_TOK);

    default:
        return new_token(UNRECOG);
    }       
}

extern token* alloc_token(token t, unsigned reg) {
    token *tokp = make(tokp, reg);
    tokp->type = t.type;
    tokp->lexeme = t.lexeme;
    tokp->file = t.file;
    tokp->line = t.line;

    return tokp;
}

extern token_list *cons_tokens(lexer *l) {
    /* initial space for the tokens list. it will shrink
       or increase depent of the final number of tokens.*/
    list *tokens = NULL;
    list *error_tokens = NULL;
    bool been_error = false;
    token curr_tok = cons_token(l);

    while (curr_tok.type != EOF_TOK) {
        token *tp = alloc_token(curr_tok, R_FIRS);
        if (is_errtok(tp)) {
            been_error = true;
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
    "INT", "FLOAT", "STRING",
    "R_STRING", "FALSE", "TRUE", "OF",
    "NIL", "MODULE", "FN", "TYPE",
    "RETURN", "LET", "FIN", "USE",
    "DO", "END", "IF", "THEN",
    "ELIF", "ELSE", "FOR", "WHILE",
    "CONTINUE", "BREAK", "RAISE",
    "HANDLE", "MATCH", "CASE",
    "IDENT", "NUM_T", "INT_T",
    "FLOAT_T", "STR_T", "BOOL_T",
    "LIST_T", "EQUAL", "AND", "OR",
    "NOT", "IN", "DOT", "DOT_DOT",
    "HASH", "AT", "COLON", "EQUAL_GREAT",
    "GREAT_PIPE", "PLUS", "MINUS",
    "ASTERISK", "SLASH", "PERCENT",
    "LESS", "GREAT", "EQUAL_EQUAL",
    "BANG_EQUAL", "LESS_EQUAL",
    "GREAT_EQUAL", "PIPE", "AMPERSAND",
    "CATER", "TILDE", "GREAT_GREAT",
    "LESS_LESS", "LPAREN", "RPAREN",
    "LBRACE", "RBRACE", "LBRACKET",
    "RBRACKET", "COMMA", "UNRECOG",
    "UNTERMIN_COMM",
    "UNTERMIN_STR", "UNTERMIN_RSTR",
    "INVALID_ESCP", "OCT_OUTOFR_ESCP",
    "OCT_MISS_ESCP", "OCT_INVL_ESCP",
    "HEX_MISS_ESCP", "HEX_INVL_ESCP",
    "INVALID_SCIEN", "EOF_TOK"
};

void print_token(token *t) {
    printf("%s@line %ld :: %s :: %s\n",
           t->file,
           t->line,
           t->lexeme,
           tok_types_str[t->type]);
}
