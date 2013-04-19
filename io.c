/* io.c: The top-level user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include "iotext.h"
#include "iocurses.h"
#include "iosdl.h"
#include "io.h"

/* Pointers to the I/O functions.
 */
void (*render)(void);
int (*getinputevent)(int *control, int *action);

/* Select the I/O platform to use and allow it to initialize.
 */
int initializeio(int iomode)
{
    switch (iomode) {
      case io_text:
	getinputevent = text_getinputevent;
	render = text_render;
	return text_initializeio();
#ifdef INCLUDE_CURSES
      case io_curses:
	getinputevent = curses_getinputevent;
	render = curses_render;
	return curses_initializeio();
#endif
#ifdef INCLUDE_SDL
      case io_sdl:
	getinputevent = sdl_getinputevent;
	render = sdl_render;
	return sdl_initializeio();
#endif
    }
    return 0;
}
