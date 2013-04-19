/* gen.c: General all-purpose functions.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "gen.h"

char const *versioninfo[] = {
    "yahtzee: version 1.5.",
    "Copyright (C) 2010 Brian Raiter.",
    "This program is free software. See README for details.",
    NULL
};

void croak(char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    exit(EXIT_FAILURE);
}

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
