/* iotext.h: The dumb text terminal user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#ifndef _iotext_h_
#define _iotext_h_

/* The pure-text-based versions of the functions defined in io.h.
 */
extern int text_initializeio(void);
extern int text_runio(int *control);

#endif
