/* gen.c: General all-purpose functions.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "gen.h"

/* Version, copyright, and license text.
 */
char const *licenseinfo[] = {
    "yahtzee: version 1.5. Copyright (C) 2010 Brian Raiter.",
    "",
    "Permission is hereby granted, free of charge, to any person",
    "obtaining a copy of this software and associated documentation files",
    "(the \"Software\"), to deal in the Software without restriction,",
    "including without limitation the rights to use, copy, modify, merge,",
    "publish, distribute, sublicense, and/or sell copies of the Software,",
    "and to permit persons to whom the Software is furnished to do so,",
    "subject to the following conditions:",
    "",
    "The above copyright notice and this permission notice shall be",
    "included in all copies or substantial portions of the Software.",
    "",
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF",
    "ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED",
    "TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A",
    "PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT",
    "SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR",
    "ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN",
    "ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,",
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE",
    "USE OR OTHER DEALINGS IN THE SOFTWARE.",
    NULL
};

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
