/*
 * (resolver.h | 10 May 19 | Ahmad Maher)
 *
 * The resolver mostly responsble for the semantic analysis
 * phase which includes:
 * -> variables resolving 
 * -> check for valid 'return', 'break' and 'continue' usage
 * -> warning for unused varaibles (to be implemented)
 *
*/

#ifndef resolver_h
#define resolver_h

#include "ast.h"
#include "eval.h"

/* semantic errors */
typedef struct SError {
    char *msg;
    Token where;
} SError;

typedef struct Resolver {
    Evaluator *evaluator;
    int been_error;          /* error flag */
    unsigned state;          /* the current state of the resolver */
    ARRAY(Table*) scopes;    /* stack of scopes */
    ARRAY(SError) errors;    /* resolving errors */
} Resolver;

/**
 * initialize a resolver with evaluator 'e',
 * and initialize a the internal arrays
*/
void init_resolver(Resolver *r, Evaluator *e);

/**
 * dispose any resources allocated by the resolver.
 * the resolver can't be used after this function.
 * init_resolver need to be recalled.
*/
void free_resolver(Resolver *r);

/* resolve an AST_piece, and return 0 on success and 
   any other integer on errors */
int resolve(Resolver *r, AST_piece p);

/* dump resulting errors from the resolving to 'out' stream */
void resolver_log(Resolver *r, FILE *out);

#endif
