/*
 * (builtins.c | 2 July 19 | Ahmad Maher)
 *
 * Bultins Functions
 * 
*/

#include <stdio.h>
#include <string.h>

#include "builtin.h"
#include "eval.h"
#include "object.h"
#include "strutil.h"

/* return the number of all elements in each table of a hash object */
static int hash_elems(Rav_obj *hash) {
    int elems = 0;
    elems += hash->float_table ? table_elems(hash->float_table) : 0;
    elems += hash->int_table ? table_elems(hash->int_table) : 0;
    elems += hash->str_table ? table_elems(hash->str_table) : 0;
    elems += hash->obj_table ? table_elems(hash->obj_table) : 0;
    return elems;
}

Rav_obj *Rav_len(Rav_obj **objects) {
    Rav_obj *result;
    
    switch (objects[0]->type) {
    case STR_OBJ: 
        result = new_object(INT_OBJ, 0);
        result->i = objects[0]->len;
        break;

    case LIST_OBJ:
        result = new_object(INT_OBJ, 0);
        result->i = list_len(objects[0]->l);
        break;

    case HASH_OBJ:
        result = new_object(INT_OBJ, 0);
        result->i = hash_elems(objects[0]);
        break;

    default:
        return rt_err("'len': applied to a non-collection");
    }

    return result;
}

Rav_obj *Rav_typ(Rav_obj **objects) {
    Rav_obj *result = new_object(STR_OBJ, 0);
    result->str = object_type(objects[0]);
    result->len = strlen(result->str);
    
    return result;
}

/** Printing Functions **/

static void print_objects(Rav_obj **objects, char sep) {
    for (int i = 0; objects[i]; i++) {
        print_object(objects[i]);
        putchar(sep);
    }
}

Rav_obj *Rav_print(Rav_obj **objects) {
    print_objects(objects, ' ');
    ungetc(' ', stdin);
    putchar('\n');
    return RNil;
}

Rav_obj *Rav_println(Rav_obj **objects) {
    print_objects(objects, '\n');
    return RNil;
}


/** List Functions **/

Rav_obj *Rav_hd(Rav_obj **objects) {
    if (objects[0]->type != LIST_OBJ) {
        return rt_err("'hd': bad argument type, list is expected");
    }

    if (!objects[0]->l) {
        return rt_err("passed empty list to 'hd'");
    }
    
    return objects[0]->l->head;
}

Rav_obj *Rav_tl(Rav_obj **objects) {
    if (objects[0]->type != LIST_OBJ) {
        return rt_err("tl: bad argument type, list is expected");
    }

    if (!objects[0]->l) {
        return rt_err("passed empty list to 'tl'");
    }
    
    Rav_obj *tail = new_object(LIST_OBJ, 0);
    tail->l = objects[0]->l->tail;
    return tail;
}
