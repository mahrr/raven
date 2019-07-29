/*
 * (builtins.c | 2 July 19 | Ahmad Maher)
 *
 * Bultins Functions
 * 
*/

#include <stdio.h>

#include "builtin.h"
#include "object.h"
#include "strutil.h"

extern Rav_obj False_obj;
extern Rav_obj True_obj;
extern Rav_obj Nil_obj;
extern Rav_obj Void_obj;

#define RTrue  (Rav_obj *)(&True_obj)
#define RFalse (Rav_obj *)(&False_obj)
#define RNil   (Rav_obj *)(&Nil_obj)
#define RVoid  (Rav_obj *)(&Void_obj)


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
        //TODO: Error Handling
        fprintf(stderr,
                "Type Error: hd: bad argument type, list is expected\n");
        return RVoid;
    }

    if (!objects[0]->l) {
        //TODO: Error Handling
        fprintf(stderr,
                "Error: passed empty list to 'hd'\n");
        return RVoid;
    }
    
    return objects[0]->l->head;
}

Rav_obj *Rav_tl(Rav_obj **objects) {
    if (objects[0]->type != LIST_OBJ) {
        //TODO: Error Handling
        fprintf(stderr,
                "Type Error: tl: bad argument type, list is expected\n");
        return RVoid;
    }

    if (!objects[0]->l) {
        //TODO: Error Handling
        fprintf(stderr,
                "Error: passed empty list to 'tl'\n");
        return RVoid;
    }
    
    Rav_obj *tail = new_object(LIST_OBJ, 0);
    tail->l = objects[0]->l->tail;
    return tail;
}
