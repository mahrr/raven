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
    if (str->s->israw) {
        fwrite(str->s->str, 1, str->s->len, stdout);
        return;
    }

    /* escaped string */
    size_t slen = str->s->len;
    char *escaped = malloc(slen);

    int elen = strescp(str->s->str, escaped, slen, NULL);
    fwrite(escaped, 1, elen, stdout);
    
    free(escaped);
}

static void print_variant(Rav_obj *variant) {
    printf("%s(", variant->vr->cons->cn->name);
    
    Rav_obj **objects = variant->vr->elems;
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
        fwrite(object->s->str, 1, object->s->len, stdout);
        putchar('\'');
    } else
        print_object(object);
}

void print_object(Rav_obj *object) {
    switch(object->type) {
    case BOOL_OBJ:
        printf("%s", object->b ? "true" : "false");
        break;
    case BLTIN_OBJ:
        printf("<built-in>");
        break;
    case CLOS_OBJ:
        printf("<closure>/%d", object->cl->arity);
        break;
    case FLOAT_OBJ:
        printf("%.16g", object->f);
        break;
    case HASH_OBJ:
        print_hash(object);
        break;
    case LIST_OBJ:
        print_list(object);
        break;
    case INT_OBJ:
        printf("%ld", object->i);
        break;
    case NIL_OBJ:
        printf("nil");
        break;
    case STR_OBJ:
        print_str(object);
        break;
    case VARI_OBJ:
        print_variant(object);
        break;
    case VOID_OBJ:
        break;

    default:
        fprintf(stderr, "[INTERNAL] invalid object type (%d)\n",
                object->type);
        assert(0);
    }
}

Rav_obj *new_object(Rav_type type, uint8_t mode) {
    Rav_obj *object = malloc(sizeof(*object));
    object->type = type;
    object->mode = mode;
    return object;
}
