/* gen.c: General all-purpose functions.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "gen.h"

/* Exit the program with an error message.
 */
void croak(char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    exit(EXIT_FAILURE);
}

/* Allocate memory or exit.
 */
void *allocate(unsigned int size)
{
    void *p;

    p = malloc(size);
    if (!p) {
	fputs("memory allocation failed\n", stderr);
	exit(EXIT_FAILURE);
    }
    return p;
}

/* Find the next place to break a word-wrapped string of text,
 * automatically skipping leading whitespace.
 */
int textbreak(char const **pstr, int width)
{
    char const *str;
    int brk, n;

    while (**pstr == ' ')
	++*pstr;
    str = *pstr;
    if (*str == '\0')
	return 0;
    brk = 0;
    for (n = 1 ; n < width ; ++n) {
	if (str[n] == '\0')
	    return n;
	else if (str[n] == ' ' && str[n - 1] != ' ')
	    brk = n;
    }
    return brk ? brk : width;
}
