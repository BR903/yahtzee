/* io.c: The top-level user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include "iotext.h"
#include "iocurses.h"
#include "iosdl.h"
#include "io.h"

/* Pointer to the I/O function.
 */
int (*runio)(int *control);

/* Select the I/O platform to use and allow it to initialize.
 */
int initializeio(int iomode)
{
    switch (iomode) {
      case io_text:
	runio = text_runio;
	return text_initializeio();
#ifdef INCLUDE_CURSES
      case io_curses:
	runio = curses_runio;
	return curses_initializeio();
#endif
#ifdef INCLUDE_SDL
      case io_sdl:
	runio = sdl_runio;
	return sdl_initializeio();
#endif
    }
    return 0;
}
