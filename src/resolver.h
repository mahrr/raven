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
#include "error.h"
#include "eval.h"

typedef struct Resolver {
    Evaluator *evaluator;
    int been_error;          /* error flag */
    unsigned state;          /* the current state of the resolver */
    ARRAY(Table*) scopes;    /* stack of scopes */
    ARRAY(SErr) errors;      /* resolving errors */
} Resolver;

/*
 * initialize a resolver with evaluator 'e',
 * and initialize a the internal arrays
*/
void init_resolver(Resolver *r, Evaluator *e);

/*
 * dispose any resources allocated by the resolver.
 * the resolver can't be used after this function.
 * init_resolver need to be recalled.
*/
void free_resolver(Resolver *r);

/*
 * resolve an AST_piece, and return 0 on success and 
 * any other integer on errors.
*/
int resolve(Resolver *r, AST_piece p);

/* 1 if there is a semantic error, 0 otherwise. */
#define resolve_error(r) ((r)->been_error)

/* a pointer to the array of the resolver errors. */
#define resolver_errors(r) ((r)->errors.elems)

/* number of semantic errors occurs during resolving. */
#define resolver_errnum(r) ((r)->errors.len);

#endif
