/* gen.h: General all-purpose functions.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#ifndef _gen_h_
#define _gen_h_

/* Display a formatted error message and exit the program.
 */
extern void croak(char const *fmt, ...);

/* Wrapper around malloc that aborts if memory is unavailable.
 */
extern void *allocate(unsigned int size);

/* Return the longest prefix of str that fits in width bytes while
 * breaking at a space (if possible).
 */
extern int textbreak(char const **pstr, int width);

#endif
