/* iocurses.h: The curses user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#ifndef _iocurses_h_
#define _iocurses_h_

#ifdef INCLUDE_CURSES

/* The curses-based versions of the functions defined in io.h.
 */
extern int curses_initializeio(void);
extern int curses_runio(int *control);

#endif

#endif
