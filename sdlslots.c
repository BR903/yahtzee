/* slots.c: An SDL slot control.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include "SDL.h"
#include "SDL_ttf.h"
#include "yahtzee.h"
#include "gen.h"
#include "iosdlctl.h"

/* The various states a slot can be in.
 */
enum { s_open, s_set, s_over, s_selected, s_count };

/* The text labels corresponding to each possible slot.
 */
static char const *titles[ctl_slots_count] = {
    "Ones", "Twos", "Threes", "Fours", "Fives", "Sixes", "Subtotal",
    "Bonus", "Three of a Kind", "Four of a Kind", "Full House",
    "Small Straight", "Large Straight", "Yahtzee", "Chance", "Total Score"
};

/* The various colors to use when rendering a slot: the background
 * color, the basic text color, and the dim text color.
 */
static SDL_Color const bkgndcolor = { 255, 255, 255, 0 };
static SDL_Color const textcolor = { 0, 0, 0, 0 };
static SDL_Color const dimtextcolor = { 191, 191, 191, 0 };

/* Reference count for shared objects.
 */
static int ctlrefcount = 0;

/* The slot font.
 */
static TTF_Font *font;

/* The font's height in pixels.
 */
static int fontheight;

/* The dimensions of a slot.
 */
static int slotwidth, slotheight;

/* The size of the slot's border.
 */
static int bordersize;

/* The space to leave between the slot's label and the score.
 */
static int spacerwidth;

/*
 * Drawing functions.
 */

/* Load the font and measure the dimensions of a slot and its
 * components.
 */
static void initfont(void)
{
    int w, i;

    fontheight = 4 * sdl_scalingunit;
    font = TTF_OpenFont(FONT_MED_PATH, fontheight);
    if (!font)
	croak("%s\nUnable to load font.", TTF_GetError());

    bordersize = (3 * fontheight) / 16;
    slotwidth = 0;
    for (i = 0 ; i < ctl_slots_count ; ++i) {
	TTF_SizeUTF8(font, titles[i], &w, &slotheight);
	if (slotwidth < w)
	    slotwidth = w;
    }
    TTF_SizeUTF8(font, "888", &w, NULL);
    TTF_SizeUTF8(font, "|#", &spacerwidth, NULL);
    slotwidth += w + 2 * bordersize + 3 * spacerwidth;
    slotheight += 2 * bordersize;
}

/* Given a surface, draw a border around its edges, using the given
 * color and width (in pixels).
 */
static void outlinesurface(SDL_Surface *surface, SDL_Color color, int width)
{
    Uint32 colorvalue;
    SDL_Rect rect;

    colorvalue = SDL_MapRGB(surface->format, color.r, color.g, color.b);
    rect.x = 0;
    rect.y = 0;
    rect.w = surface->w;
    rect.h = width;
    SDL_FillRect(surface, &rect, colorvalue);
    rect.y = surface->h - width;
    SDL_FillRect(surface, &rect, colorvalue);
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = surface->h;
    SDL_FillRect(surface, &rect, colorvalue);
    rect.x = surface->w - width;
    SDL_FillRect(surface, &rect, colorvalue);
}

/* Create the basic images for the given slot control. These images
 * include the label associated with the given control ID.
 */
static int makeslotimages(struct sdlcontrol *ctl, int slotid)
{
    SDL_Surface *image;
    SDL_Rect rect;

    if (!font)
	initfont();

    image = SDL_CreateRGBSurface(SDL_SWSURFACE, slotwidth, slotheight, 32,
				 0x0000FF, 0x00FF00, 0xFF0000, 0);
    SDL_FillRect(image, NULL, 0xFFFFFF);
    ctl->images[s_open] = SDL_DisplayFormat(image);
    SDL_FreeSurface(image);
    outlinesurface(ctl->images[s_open], textcolor, 1);
    image = TTF_RenderUTF8_Shaded(font, titles[slotid - ctl_slots],
				  textcolor, bkgndcolor);
    rect.x = bordersize + spacerwidth;
    rect.y = bordersize;
    SDL_BlitSurface(image, NULL, ctl->images[s_open], &rect);
    SDL_FreeSurface(image);

    ctl->images[s_over] = SDL_DisplayFormat(ctl->images[s_open]);
    ctl->images[s_set] = SDL_DisplayFormat(ctl->images[s_open]);
    ctl->images[s_selected] = SDL_DisplayFormat(ctl->images[s_open]);
    return 1;
}

/* Modify the control's images to show the current score value stored
 * for that slot.
 */
static int updateslotimages(struct sdlcontrol *ctl)
{
    SDL_Surface *image;
    SDL_Rect rect;
    char buf[16];

    if (ctl->lastvalue < 0)
	sprintf(buf, " ");
    else
	sprintf(buf, "%d", ctl->lastvalue);

    SDL_BlitSurface(ctl->images[s_open], NULL, ctl->images[s_over], NULL);
    image = TTF_RenderUTF8_Shaded(font, buf, dimtextcolor, bkgndcolor);
    rect.x = slotwidth - bordersize - spacerwidth - image->w;
    rect.y = bordersize;
    SDL_BlitSurface(image, NULL, ctl->images[s_over], &rect);
    SDL_FreeSurface(image);

    SDL_BlitSurface(ctl->images[s_open], NULL, ctl->images[s_set], NULL);
    image = TTF_RenderUTF8_Shaded(font, buf, textcolor, bkgndcolor);
    SDL_BlitSurface(image, NULL, ctl->images[s_set], &rect);
    SDL_FreeSurface(image);

    SDL_BlitSurface(ctl->images[s_set], NULL, ctl->images[s_selected], NULL);
    outlinesurface(ctl->images[s_selected], textcolor, bordersize);

    return 1;
}

/* Update the control's state. Return true if the control should be
 * redrawn, either due to a change of the state or the score value.
 */
int updateslot(struct sdlcontrol *ctl)
{
    int state, retval = 0;

    if (ctl->lastvalue != ctl->control->value) {
	ctl->lastvalue = ctl->control->value;
	updateslotimages(ctl);
	retval = 1;
    }
    if (ctl->lastvalue < 0)
	state = s_open;
    else if (isdisabled(*ctl->control))
	state = s_set;
    else if (isselected(*ctl->control))
	state = s_selected;
    else if (ctl->hovering)
	state = s_over;
    else
	state = s_open;
    if (ctl->state != state)
	retval = 1;
    ctl->state = state;
    return retval;
}

/* Initialize the given control as a slot.
 */
int makeslot(struct sdlcontrol *ctl, int slotid)
{
    ctl->images = allocate(s_count * sizeof *ctl->images);
    makeslotimages(ctl, slotid);
    updateslotimages(ctl);
    ctl->state = -1;
    ctl->lastvalue = ctl->control->value;
    ++ctlrefcount;
    return 1;
}

/* Free all resources held by this control.
 */
void unmakeslot(struct sdlcontrol *ctl)
{
    int i;

    for (i = 0 ; i < s_count ; ++i)
	SDL_FreeSurface(ctl->images[i]);
    free(ctl->images);
    --ctlrefcount;
    if (ctlrefcount == 0) {
	TTF_CloseFont(font);
	font = NULL;
    }
}
