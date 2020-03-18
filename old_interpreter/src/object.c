/*
 * (object.c | 2 July 19 | Ahmad Maher)
 *
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "object.h"
#include "strutil.h"

/** INTERNALS **/

// TODO
static void print_hash(Rav_obj *hash) {
    printf("<hash> at %p", (void*)hash);
}

static void print_list(Rav_obj *list) {
    List *cell = list->l;
    putchar('[');
    
    for ( ; cell; cell = cell->tail) {
        echo_object(cell->head);
        if (cell->tail)
            printf(", ");
    }
    
    putchar(']');
}

static void print_str(Rav_obj *str) {
    size_t slen = str->len;
    char *unescaped = malloc(slen);

    slen = strunescp(str->str, unescaped, slen, NULL);
    fwrite(unescaped, 1, slen, stdout);
    
    free(unescaped);
}

static void print_variant(Rav_obj *variant) {
    printf("%s(", variant->cons->name);
    
    Rav_obj **objects = variant->elems;
    for (int i = 0; objects[i]; i++) {
        echo_object(objects[i]);
        
        if (objects[i+1])
            printf(", ");
    }

    putchar(')');
}

/** INTERFACE **/

/* string correspond to the Rav_type enum */
static char *str_obj_types[] = {
    "boolean", "function", "function",
    "constructor", "float", "hash", "list",
    "integer", "nil", "string", "variant"
};

char *object_type(Rav_obj *object) {
    return str_obj_types[object->type];
}

void echo_object(Rav_obj *object) {
    if (object->type == STR_OBJ) {
        putchar('\'');
        fwrite(object->str, 1, object->len, stdout);
        putchar('\'');
    } else
        print_object(object);
}

void print_object(Rav_obj *object) {
    switch(object->type) {
    case BOOL_OBJ:
        printf("%s", object->b ? "true" : "false");
        return;
    case BLTIN_OBJ:
        printf("<built-in>");
        return;
    case CLOS_OBJ:
        printf("<closure>/%d", object->cl_arity);
        return;
    case CONS_OBJ:
        printf("<constructor:%s>/%d", object->name, object->cs_arity);
        return;
    case FLOAT_OBJ:
        printf("%.16g", object->f);
        return;
    case HASH_OBJ:
        print_hash(object);
        return;
    case LIST_OBJ:
        print_list(object);
        return;
    case INT_OBJ:
        printf("%ld", object->i);
        return;
    case NIL_OBJ:
        printf("nil");
        return;
    case STR_OBJ:
        print_str(object);
        return;
    case VARI_OBJ:
        print_variant(object);
        return;
    case VOID_OBJ:
        return;
    }

    
    fprintf(stderr, "[INTERNAL] invalid object type(%d)\n", object->type);
    assert(0);
}

Rav_obj *new_object(Rav_type type, uint8_t mode) {
    Rav_obj *object = malloc(sizeof(*object));
    object->type = type;
    object->mode = mode;
    return object;
}
