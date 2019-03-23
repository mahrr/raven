/*
 * (strutil.h | 12 Dec 18 | Ahmad Maher)
 *
 * String utilities
 *
*/

#ifndef strutil_h
#define strutil_h

/* escape a literal string *unscaped. return NULL on success, 
   and a pointer to the invalid escape on failure. */
extern char *escape(char *unescaped, char *escaped, int size);

/* convert n to a decimal representation string and returns it */
extern char *strd(long n);

#endif
