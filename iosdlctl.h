/* iosdlctl.h: The SDL user interface internals.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#ifndef _iosdlctl_h_
#define _iosdlctl_h_

#include "SDL.h"

/* The default locations of our fonts. Normally the location is
 * supplied in the Makefile via the compiler command-line.
 */
#ifndef FONT_MED_PATH
#define FONT_MED_PATH "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
#endif
#ifndef FONT_BOLD_PATH
#define FONT_BOLD_PATH "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf"
#endif

struct control;

/* The SDL-specific information stored for each I/O control.
 */
struct sdlcontrol {
    struct control const *control;	/* pointer to the control info */
    SDL_Rect rect;			/* where the control appears */
    SDL_Surface **images;		/* renderings for all states */
    int state;				/* the control's current state */
    int lastvalue;			/* value used to make the images */
    int hovering;			/* true if the ctl is moused over */
    int down;				/* true if the mouse button is down */
};

/* The display.
 */
extern SDL_Surface *sdl_screen;

/* The scaling unit.
 */
extern int sdl_scalingunit;

/* The color of the window's background area.
 */
extern SDL_Color const sdl_bkgndcolor;

/* Functions to initialize a control as a specific type: a die, a
 * slot, or a button. Slot controls need to know their ID value in
 * order to choose their label, and dice controls need to know the
 * color of the window background.
 */
extern int makedie(struct sdlcontrol *ctl, SDL_Color bkgnd);
extern int makebutton(struct sdlcontrol *ctl);
extern int makeslot(struct sdlcontrol *ctl, int slotid);

/* Functions to update a control's state. Each function returns true
 * if the control needs to be redrawn.
 */
extern int updatedie(struct sdlcontrol *ctl);
extern int updatebutton(struct sdlcontrol *ctl);
extern int updateslot(struct sdlcontrol *ctl);

/* Functions to free resources associated with a control.
 */
extern void unmakedie(struct sdlcontrol *ctl);
extern void unmakebutton(struct sdlcontrol *ctl);
extern void unmakeslot(struct sdlcontrol *ctl);

/* Functions to display online help.
 */
extern int runhelp(void);
extern int showkeyhelp(struct sdlcontrol const *sdlcontrols);

#endif
