/* io.h: The top-level user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#ifndef _io_h_
#define _io_h_

/* The list of available I/O platforms.
 */
enum { io_text, io_curses, io_sdl };

/* Prepare the I/O subsystem. Returns false if a error occurred.
 */
extern int initializeio(int);

/* Update the output to reflect the current game state and wait for an
 * input event. Returns false if the user requested that the program
 * exit. Otherwise returns true, and with control set to the control
 * id that received the input.
 */
extern int (*runio)(int *control);

#endif
