/* gen.h: General all-purpose functions.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#ifndef _gen_h_
#define _gen_h_

/* Null-terminated array of lines of text, giving the program's
 * version number, copyright, and license.
 */
extern char const *licenseinfo[];

/* Display a formatted error message and exit the program.
 */
extern void croak(char const *fmt, ...);

/* Wrapper around malloc that aborts if memory is unavailable.
 */
extern void *allocate(unsigned int size);

#endif
