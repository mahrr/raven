/*
 * (strutil.h | 12 Dec 18 | Ahmad Maher)
 *
 * String utilities.
*/

#ifndef strutil_h
#define strutil_h

/* escaping errors */
#define INV_ESCP 0x1
#define OCT_OFRG 0x2
#define OCT_MISS 0x4
#define OCT_INVL 0x8
#define HEX_MISS 0x16
#define HEX_INVL 0x32

/* escape a literal string unscaped, put it in escaped.
   return state indicating escaping error if found or */
extern int escape(char *unescaped, char *escaped, int size);

/* convert n to a decimal representation string and returns it */
extern char *strd(long n);

#endif
