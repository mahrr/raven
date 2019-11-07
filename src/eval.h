/*
 * (eval.h | 26 April 19 | Kareem Hamdy, Amr Anwar)
 *
 * Raven syntax tree interpreter interface.
 *
*/

#ifndef eval_h
#define eval_h

#include "ast.h"
#include "object.h"
#include "table.h"

typedef struct Evaluator {
    Table *global;   /* <string: object> global environment  */
    Env *current;    /* current local environment */
} Evaluator;


/*
 * Initialize an evaluator state, allocate a space for
 * global environment,and it set  the current environment
 * to the global.
 * 
*/
void init_eval(Evaluator *e);

/*
 * dispose the evaluator internal allocated space.
 * the evaluator can't be used after applying this
 * function, init_eval should be called first.
*/
void free_eval(Evaluator *e);

/*
 * walk the whole piece syntax tree, sequentially 
 * executing its statements. it returns the result
 * of the last statement execution.
*/
Rav_obj *walk(Evaluator *e, AST_piece piece);

/*
 * execute statement, if it's an expression statement,
 * it will return the result of evaluating the statement
 * expression, otherwise an empty object that will be 
 * discarded.
*/
Rav_obj *execute(Evaluator *e, AST_stmt stmt);

/*
 * evaluate an expression. it returns the result raven
 * object from evaluation the expression.
*/
Rav_obj *eval(Evaluator *e, AST_expr expr);


#endif
