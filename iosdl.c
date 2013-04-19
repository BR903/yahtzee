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

/* The scaling unit. Changing this changes the size of everything
 * (except the dice, which are hardcoded images).
 */
int const sdl_scalingunit = 4;

/* The color of the window's background area.
 */
static SDL_Color const bkgndcolor = { 128, 191, 191, 0 };

/* The array of SDL control info, mirroring the controls array.
 */
static struct sdlcontrol sdlcontrols[ctl_count];

/* Size of the display.
 */
static int cxWindow, cyWindow;

/* True if the window needs to be rendered from scratch.
 */
static int redrawall;

/* Each control's state update functions.
 */
static int (*stateupdatefunctions[ctl_count])(struct sdlcontrol *);

/* A small event queue. Needed because a single SDL event
 * can potentially map to two or even three input events.
 */
static struct { int control, action; } eventqueue[4];
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
	makedie(&sdlcontrols[i]);
	stateupdatefunctions[i] = updatedie;
    }
    makebutton(&sdlcontrols[ctl_button]);
    stateupdatefunctions[i] = updatebutton;
    for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
	makeslot(&sdlcontrols[i], i);
	stateupdatefunctions[i] = updateslot;
    }
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

/*
 * Using the event queue.
 */

/* Add an input event to the queue.
 */
static void queueinputevent(int control, int action)
{
    if (eventqueuesize < sizeof eventqueue / sizeof *eventqueue) {
	eventqueue[eventqueuesize].control = control;
	eventqueue[eventqueuesize].action = action;
	++eventqueuesize;
    }
}

/* Retrieve the first input event from the queue. Returns false if the
 * queue is empty.
 */
static int unqueueinputevent(int *control, int *action)
{
    if (eventqueuesize == 0)
	return 0;
    *control = eventqueue[0].control;
    *action = eventqueue[0].action;
    --eventqueuesize;
    memmove(eventqueue, eventqueue + 1, eventqueuesize * sizeof *eventqueue);
    return 1;
}

/*
 * Exported functions.
 */

/* Called when the program is exiting.
 */
static void shutdown(void)
{
    if (TTF_WasInit())
	TTF_Quit();
    if (SDL_WasInit(SDL_INIT_VIDEO))
	SDL_Quit();
}

/* Create the SDL display and initialize everything.
 */
int sdl_initializeio(void)
{
    atexit(shutdown);
    if (SDL_Init(SDL_INIT_VIDEO))
	croak("%s\nCannot initialize SDL.", SDL_GetError());
    if (TTF_Init())
	croak("%s\nCannot initialize SDL_ttf.", TTF_GetError());

    sdl_screen = SDL_SetVideoMode(1, 1, 0, SDL_SWSURFACE | SDL_ANYFORMAT);
    if (!sdl_screen)
	croak("%s\nCannot initialize display.", SDL_GetError());
    initcontrols();
    initlayout();
    sdl_screen = SDL_SetVideoMode(cxWindow, cyWindow, 0,
				  SDL_SWSURFACE | SDL_ANYFORMAT);
    if (!sdl_screen)
	croak("%s\nCannot resize display to %d x %d.",
	      SDL_GetError(), cxWindow, cyWindow);

    SDL_WM_SetCaption("Yahtzee", "Yahtzee");
    SDL_EnableUNICODE(1);
    redrawall = 1;
    return 1;
}

/* Render our controls to the display as required.
 */
void sdl_render(void)
{
    SDL_Rect drawrects[ctl_count];
    Uint32 color;
    int drawcount, i;

    if (redrawall) {
	color = SDL_MapRGB(sdl_screen->format, bkgndcolor.r,
			   bkgndcolor.g, bkgndcolor.b);
	SDL_FillRect(sdl_screen, NULL, color);
    }
    drawcount = 0;
    for (i = 0 ; i < ctl_count ; ++i) {
	if (stateupdatefunctions[i](&sdlcontrols[i]) || redrawall) {
	    if (sdlcontrols[i].images[sdlcontrols[i].state]->format->Amask)
		SDL_FillRect(sdl_screen, &sdlcontrols[i].rect,
			     SDL_MapRGB(sdl_screen->format, bkgndcolor.r,
					bkgndcolor.g, bkgndcolor.b));
	    SDL_BlitSurface(sdlcontrols[i].images[sdlcontrols[i].state], NULL,
			    sdl_screen, &sdlcontrols[i].rect);
	    drawrects[drawcount++] = sdlcontrols[i].rect;
	}
    }
    if (redrawall) {
	SDL_UpdateRect(sdl_screen, 0, 0, 0, 0);
	redrawall = 0;
    } else {
	if (drawcount)
	    SDL_UpdateRects(sdl_screen, drawcount, drawrects);
    }
}

/* Run the event loop until a valid input event is encountered. Input
 * events include mouse hovering onto (or out of) a control, left
 * mouse button clicks (up and down), and keyboard characters
 * corresponding to a control's hotkey.
 */
int sdl_getinputevent(int *control, int *action)
{
    static int mousetrap = -1;
    static int lasthovering = -1;

    SDL_Event event;
    int ch, i;

    for (;;) {
	if (unqueueinputevent(control, action))
	    return 1;
	if (SDL_WaitEvent(&event) < 0)
	    exit(1);
	switch (event.type) {
	  case SDL_MOUSEBUTTONDOWN:
	    if (event.button.button != SDL_BUTTON_LEFT)
		break;
	    if (mousetrap >= 0) {
		queueinputevent(mousetrap, act_out);
		queueinputevent(mousetrap, act_up);
		mousetrap = -1;
	    }
	    i = ctl_count;
	    while (i-- > 0 && !inrect(event.button, sdlcontrols[i].rect)) ;
	    lasthovering = i;
	    if (i >= 0) {
		queueinputevent(i, act_down);
		mousetrap = i;
	    }
	    break;
	  case SDL_MOUSEMOTION:
	    if (mousetrap >= 0) {
		if (inrect(event.button, sdlcontrols[mousetrap].rect)) {
		    if (lasthovering != mousetrap) {
			lasthovering = mousetrap;
			queueinputevent(mousetrap, act_over);
		    }
		} else {
		    if (lasthovering == mousetrap) {
			lasthovering = -1;
			queueinputevent(mousetrap, act_out);
		    }
		}
	    } else {
		i = ctl_count;
		while (i-- && !inrect(event.button, sdlcontrols[i].rect)) ;
		if (lasthovering != i) {
		    queueinputevent(lasthovering, act_out);
		    lasthovering = i;
		    if (i >= 0)
			queueinputevent(i, act_over);
		}
	    }
	    break;
	  case SDL_MOUSEBUTTONUP:
	    if (event.button.button != SDL_BUTTON_LEFT)
		break;
	    if (mousetrap < 0)
		break;
	    if (!inrect(event.button, sdlcontrols[mousetrap].rect))
		queueinputevent(mousetrap, act_out);
	    queueinputevent(mousetrap, act_up);
	    mousetrap = -1;
	    break;
	  case SDL_KEYDOWN:
	    if ((event.key.keysym.mod & KMOD_CTRL) &&
				event.key.keysym.sym == SDLK_w)
		return 0;
	    if ((event.key.keysym.mod & KMOD_ALT) &&
				event.key.keysym.sym == SDLK_F4)
		return 0;
	    if (event.key.keysym.sym == SDLK_RETURN ||
				event.key.keysym.sym == SDLK_KP_ENTER) {
		queueinputevent(ctl_button, act_clicked);
		break;
	    }
	    if (event.key.keysym.sym == SDLK_F1 ||
				event.key.keysym.unicode == '?') {
		if (!runhelp())
		    return 0;
		redrawall = 1;
		sdl_render();
		break;
	    } else if ((event.key.keysym.mod & KMOD_CTRL) &&
				event.key.keysym.sym == SDLK_k) {
		if (!showkeyhelp(sdlcontrols))
		    return 0;
		redrawall = 1;
		sdl_render();
		break;
	    }
	    ch = event.key.keysym.unicode;
	    if (ch) {
		for (i = 0 ; i < ctl_count ; ++i) {
		    if (sdlcontrols[i].control->key == ch) {
			queueinputevent(i, act_clicked);
			break;
		    }
		}
	    }
	    break;
	  case SDL_VIDEOEXPOSE:
	    redrawall = 1;
	    sdl_render();
	    break;
	  case SDL_QUIT:
	    return 0;
	  default:
	    break;
	}
    }

}
