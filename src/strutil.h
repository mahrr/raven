/*
 * (strutil.h | 12 Dec 18 | Ahmad Maher)
 *
 * String utilities
 *
*/

#ifndef strutil_h
#define strutil_h

/* 
 * it mimcs 'strdup' of glibc behaviour.
 * return a pointer to a duplicated string of s.
 * the duplicated string is allocated by malloc,
 * and can be freed with free.
*/
char *strdup(const char *s);

/*
 * it mimcs 'strndup' of glibc behaviour.
 * it's similiar to strdup, but copies n bytes at most
 * or less if s is less than n bytes. if s is longer 
 * than n bytes, only n are copied and terminating null
 * byte is added.
 *
*/
char *strndup(const char *s, size_t n);


/*
 * returns a newly allocated escaped string of 's',
 * provided its size.
*/
char *strescp(const char *s, size_t size);


/* 
 * unescapes a literal string s to the presumably allocated 
 * space pointed by 'unescaped' pointer which the function
 * assumes it point to enough space to hold the original
 * string. return the size of the unescaped string on success, 
 * and -1 on invalid escape sequence. Given 'end' is not
 * NULL, the function stores a pointer to the beginning
 * of the invalid escape sequence in the location refrenced
 * by 'end'.
*/
size_t strunescp(const char *s, char *unescaped, size_t size, char **end);

/*
 * Like strunescp, but only calculate the size of the string 
 * after unescaping, and returns it. It doesn't check for any
 * escaping errors, so passing a malformed string will result
 * in an incorrect size.
*/
size_t strunescp_len(const char *s, size_t size);

#endif
