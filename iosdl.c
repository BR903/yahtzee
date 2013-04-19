/* iosdl.c: The SDL user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include <stdlib.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "yahtzee.h"
#include "gen.h"
#include "iosdlctl.h"
#include "iosdl.h"

/* True if the point is inside the rectangle.
 */
#define inrect(p, r) ((p).x >= (r).x && (p).x < (r).x + (r).w && \
		      (p).y >= (r).y && (p).y < (r).y + (r).h)

/* The display.
 */
SDL_Surface *sdl_screen;

/* The scaling unit. Changing this changes the size of everything.
 */
int sdl_scalingunit = 4;

/* The color of the window's background area.
 */
static SDL_Color const bkgnd = { 128, 191, 191, 0 };
static Uint32 bkgndcolor;

/* Size of the display.
 */
static int cxWindow, cyWindow;

/* True if the window needs to be rendered from scratch.
 */
static int redrawall;

/* The array of SDL control info, mirroring the controls array.
 */
static struct sdlcontrol sdlcontrols[ctl_count];

/* Each control's state update functions.
 */
static int (*stateupdatefunctions[ctl_count])(struct sdlcontrol *);

/* A small event queue. Needed because a single SDL event
 * can potentially map to two or even three input events.
 */
static int eventqueue[4];
static unsigned int eventqueuesize = 0;

/*
 * Display initialization.
 */

/* Put the SDL controls in their initial states.
 */
static void initcontrols(void)
{
    int i;

    for (i = 0 ; i < ctl_count ; ++i)
	sdlcontrols[i].control = &controls[i];

    for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	makedie(&sdlcontrols[i], bkgnd);
	stateupdatefunctions[i] = updatedie;
    }
    makebutton(&sdlcontrols[ctl_button]);
    stateupdatefunctions[i] = updatebutton;
    for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
	makeslot(&sdlcontrols[i], i);
	stateupdatefunctions[i] = updateslot;
    }
}

/* Free all resources associated with the SDL controls.
 */
static void uninitcontrols(void)
{
    int i;

    for (i = ctl_dice ; i < ctl_dice_end ; ++i)
	unmakedie(&sdlcontrols[i]);
    unmakebutton(&sdlcontrols[ctl_button]);
    for (i = ctl_slots ; i < ctl_slots_end ; ++i)
	unmakeslot(&sdlcontrols[i]);
}

/* Determine the display locations of all of the SDL controls.
 */
static void initlayout(void)
{
    int cxSpacing, cySpacing, cxDieSpacing;
    int i, n, x, y;

    for (i = 0 ; i < ctl_count ; ++i) {
	sdlcontrols[i].rect.w = sdlcontrols[i].images[0]->w;
	sdlcontrols[i].rect.h = sdlcontrols[i].images[0]->h;
    }

    cxSpacing = sdl_scalingunit * 4;
    cySpacing = sdl_scalingunit * 4;
    cxDieSpacing = sdl_scalingunit;

    x = sdlcontrols[ctl_dice].rect.w * 5 + cxDieSpacing * 4;
    cxWindow = sdlcontrols[ctl_slots].rect.w * 2;
    if (cxWindow < x)
	cxWindow = x;
    cxWindow += cxSpacing * 2;

    x = (cxWindow - x) / 2;
    y = cySpacing;
    for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	sdlcontrols[i].rect.x = x;
	sdlcontrols[i].rect.y = y;
	x += sdlcontrols[ctl_dice].rect.w + cxDieSpacing;
    }

    y += sdlcontrols[ctl_dice].rect.h + cySpacing;
    sdlcontrols[ctl_button].rect.x =
	(cxWindow - sdlcontrols[ctl_button].rect.w) / 2;
    sdlcontrols[ctl_button].rect.y = y;

    x = (cxWindow - sdlcontrols[ctl_slots].rect.w * 2) / 2;
    y += sdlcontrols[ctl_button].rect.h + cySpacing;
    for (n = 0 ; n < ctl_slots_count / 2 ; ++n) {
	i = ctl_slots + n;
	sdlcontrols[i].rect.x = x;
	sdlcontrols[i].rect.y = y;
	i += ctl_slots_count / 2;
	sdlcontrols[i].rect.x = x + sdlcontrols[ctl_slots].rect.w;
	sdlcontrols[i].rect.y = y;
	y += sdlcontrols[ctl_slots].rect.h;
    }

    cyWindow = y + cySpacing;
}

/* Lay out the display and create an appropriately-sized screen.
 */
static void createscreen(void)
{
    initcontrols();
    initlayout();
    sdl_screen = SDL_SetVideoMode(cxWindow, cyWindow, 0,
				  SDL_SWSURFACE | SDL_ANYFORMAT);
    if (!sdl_screen)
	croak("%s\nCannot resize display to %d x %d.",
	      SDL_GetError(), cxWindow, cyWindow);
    bkgndcolor = SDL_MapRGB(sdl_screen->format, bkgnd.r, bkgnd.g, bkgnd.b);
    redrawall = 1;
}

/*
 * Exported functions.
 */

/* Called when the program is exiting.
 */
static void shutdown(void)
{
    uninitcontrols();
    if (TTF_WasInit())
	TTF_Quit();
    if (SDL_WasInit(SDL_INIT_VIDEO))
	SDL_Quit();
}

/* Create the SDL display and initialize everything.
 */
int sdl_initializeio(void)
{
    if (SDL_Init(SDL_INIT_VIDEO))
	return 0;
    atexit(shutdown);
    if (TTF_Init())
	croak("%s\nCannot initialize SDL_ttf.", TTF_GetError());
    sdl_screen = SDL_SetVideoMode(1, 1, 0, SDL_SWSURFACE | SDL_ANYFORMAT);
    if (!sdl_screen)
	croak("%s\nCannot initialize display.", SDL_GetError());
    SDL_WM_SetCaption("Yahtzee", "Yahtzee");
    SDL_EnableUNICODE(1);
    createscreen();
    return 1;
}

/* Render our controls to the display as required.
 */
static void render(void)
{
    SDL_Rect drawrects[ctl_count];
    int drawcount, i;

    if (redrawall) {
	SDL_FillRect(sdl_screen, NULL, bkgndcolor);
	for (i = 0 ; i < ctl_count ; ++i) {
	    stateupdatefunctions[i](&sdlcontrols[i]);
	    SDL_BlitSurface(sdlcontrols[i].images[sdlcontrols[i].state], NULL,
			    sdl_screen, &sdlcontrols[i].rect);
	}
	SDL_UpdateRect(sdl_screen, 0, 0, 0, 0);
	redrawall = 0;
    } else {
	drawcount = 0;
	for (i = 0 ; i < ctl_count ; ++i) {
	    if (!stateupdatefunctions[i](&sdlcontrols[i]))
		continue;
	    SDL_BlitSurface(sdlcontrols[i].images[sdlcontrols[i].state], NULL,
			    sdl_screen, &sdlcontrols[i].rect);
	    drawrects[drawcount++] = sdlcontrols[i].rect;
	}
	if (drawcount)
	    SDL_UpdateRects(sdl_screen, drawcount, drawrects);
    }
}

/*
 * Managing the controls.
 */

/* Add an input event to the queue.
 */
static void queueinputevent(int control)
{
    if (eventqueuesize < sizeof eventqueue / sizeof *eventqueue)
	eventqueue[eventqueuesize++] = control;
}

/* Retrieve the first input event from the queue. Returns false if the
 * queue is empty.
 */
static int unqueueinputevent(int *control)
{
    if (eventqueuesize == 0)
	return 0;
    *control = eventqueue[0];
    --eventqueuesize;
    memmove(eventqueue, eventqueue + 1, eventqueuesize * sizeof *eventqueue);
    return 1;
}

/* Track which control the mouse is hovering over.
 */
static void sethovering(int id)
{
    int i;

    for (i = 0 ; i < ctl_count ; ++i)
	sdlcontrols[i].hovering = i == id;
}

/* Simulate a brief mouse click on a control.
 */
static void flashcontrol(int id)
{
    struct sdlcontrol saved;

    saved = sdlcontrols[id];
    sdlcontrols[id].hovering = 1;
    sdlcontrols[id].down = 1;
    render();
    SDL_Delay(100);
    sdlcontrols[id] = saved;
    render();
}

/* Update the display and run the event loop until a valid input event
 * is encountered. Input events include mouse button clicks (up and
 * down), and keyboard characters corresponding to a control's hotkey.
 */
int sdl_runio(int *control)
{
    static int mousetrap = -1;

    SDL_Event event;
    int ch, i;

    for (;;) {
	if (unqueueinputevent(control))
	    return 1;
	render();
	if (SDL_WaitEvent(&event) < 0)
	    exit(1);
	switch (event.type) {
	  case SDL_MOUSEBUTTONDOWN:
	    if (event.button.button != SDL_BUTTON_LEFT)
		break;
	    if (mousetrap >= 0) {
		sdlcontrols[mousetrap].down = 0;
		mousetrap = -1;
	    }
	    i = ctl_count;
	    while (i-- > 0 && !inrect(event.button, sdlcontrols[i].rect)) ;
	    sethovering(i);
	    if (i == ctl_button) {
		mousetrap = i;
		sdlcontrols[i].down = 1;
	    } else {
		if (i >= 0)
		    queueinputevent(i);
	    }
	    break;
	  case SDL_MOUSEMOTION:
	    if (mousetrap >= 0) {
		if (inrect(event.button, sdlcontrols[mousetrap].rect))
		    sethovering(mousetrap);
		else
		    sethovering(-1);
	    } else {
		i = ctl_count;
		while (i-- && !inrect(event.button, sdlcontrols[i].rect)) ;
		sethovering(i);
	    }
	    break;
	  case SDL_MOUSEBUTTONUP:
	    if (event.button.button != SDL_BUTTON_LEFT)
		break;
	    if (mousetrap < 0)
		break;
	    sdlcontrols[mousetrap].down = 0;
	    if (inrect(event.button, sdlcontrols[mousetrap].rect))
		queueinputevent(mousetrap);
	    else
		sethovering(-1);
	    mousetrap = -1;
	    break;
	  case SDL_KEYDOWN:
	    ch = event.key.keysym.unicode;
	    if (ch) {
		for (i = 0 ; i < ctl_count ; ++i) {
		    if (sdlcontrols[i].control->key == ch) {
			queueinputevent(i);
			break;
		    }
		}
		if (i < ctl_count)
		    break;
	    }
	    if (event.key.keysym.unicode == '\r') {
		flashcontrol(ctl_button);
		queueinputevent(ctl_button);
		break;
	    } else if (event.key.keysym.unicode == '?' ||
				event.key.keysym.sym == SDLK_F1) {
		if (!runhelp())
		    return 0;
		redrawall = 1;
		break;
	    } else if (event.key.keysym.unicode == '\013') {
		if (!showkeyhelp(sdlcontrols))
		    return 0;
		redrawall = 1;
		break;
	    } else if ((event.key.keysym.mod & KMOD_CTRL) &&
				(event.key.keysym.sym == SDLK_PLUS ||
					event.key.keysym.sym == SDLK_EQUALS)) {
		uninitcontrols();
		++sdl_scalingunit;
		createscreen();
		break;
	    } else if ((event.key.keysym.mod & KMOD_CTRL) &&
				event.key.keysym.sym == SDLK_MINUS) {
		if (sdl_scalingunit > 2) {
		    uninitcontrols();
		    --sdl_scalingunit;
		    createscreen();
		}
		break;
	    }
	    if (event.key.keysym.unicode == '\030')
		return 0;
	    if ((event.key.keysym.mod & KMOD_ALT) &&
				event.key.keysym.sym == SDLK_F4)
		return 0;
	    break;
	  case SDL_VIDEOEXPOSE:
	    redrawall = 1;
	    break;
	  case SDL_QUIT:
	    return 0;
	  default:
	    break;
	}
    }
}
