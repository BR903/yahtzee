/* iosdl.h: The SDL user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#ifndef _iosdl_h_
#define _iosdl_h_

#ifdef INCLUDE_SDL

/* The SDL-based versions of the functions defined in io.h.
 */
extern int sdl_initializeio(void);
extern int sdl_getinputevent(int *control, int *action);
extern void sdl_render(void);

#endif

#endif
