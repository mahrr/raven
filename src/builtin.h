/*
 * (builtins.h | 2 July 19 | Ahmad Maher)
 *
*/


#ifndef builtins_h
#define builtins_h

#include "object.h"

/* print a variable number of objects seprated by a space */
Rav_obj *Rav_print(Rav_obj **objects);

/* print a variable number of objects seprated by newlines */
Rav_obj *Rav_println(Rav_obj **objects);

/* return the head of a list object */
Rav_obj *Rav_hd(Rav_obj **objects);

/* return the tail of a list object */
Rav_obj *Rav_tl(Rav_obj **objects);

/* return the length of a collection (e.g. list, string, hash ...) */
Rav_obj *Rav_len(Rav_obj **objects);

#endif
