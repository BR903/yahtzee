/* button.c: An SDL button control.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include "SDL.h"
#include "SDL_ttf.h"
#include "yahtzee.h"
#include "gen.h"
#include "iosdlctl.h"

/* The various states a button can be in.
 */
enum { s_disabled, s_up, s_over, s_down, s_count };

/* The text labels associated with each button value.
 */
static char const *titles[bval_count] = { "Roll Dice", "Score", "New Game" };

/* The button font.
 */
static TTF_Font *font;

/* The font's height in pixels.
 */
static int fontheight;

/* Dimensions of the button.
 */
static int buttonwidth, buttonheight;

/*
 * Drawing functions.
 */

/* Load the font and calculate the dimensions of a button.
 */
static void initfont(void)
{
    int w, i;

    fontheight = 5 * sdl_scalingunit;
    font = TTF_OpenFont(FONT_BOLD_PATH, fontheight);
    if (!font)
	croak("%s\nUnable to load font.", TTF_GetError());

    buttonwidth = 0;
    for (i = 0 ; i < bval_count ; ++i) {
	TTF_SizeUTF8(font, titles[i], &w, &buttonheight);
	if (buttonwidth < w)
	    buttonwidth = w;
    }
    buttonwidth += buttonheight * 2;
    buttonheight *= 2;
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

/* Return a surface with the given dimensions and filled with a gray
 * gradient suggesting a convex surface. shadowdepth indicates how
 * much of a grayscale difference to move through.
 */
static SDL_Surface *makeshadedsurface(int w, int h, int shadowdepth)
{
    SDL_Color shading[256];
    SDL_Surface *surface;
    Uint8 *p;
    int offset, i;

    for (i = 0 ; i < shadowdepth ; ++i)
	shading[i].r = shading[i].g = shading[i].b = 255 - i;

    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
    SDL_SetColors(surface, shading, 0, shadowdepth);

    if (SDL_MUSTLOCK(surface))
	SDL_LockSurface(surface);
    offset = h / 4;
    for (i = 0, p = surface->pixels ; i < h ; ++i, p += surface->pitch)
	memset(p, (abs(i - offset) * shadowdepth) / (h - offset), w);
    if (SDL_MUSTLOCK(surface))
	SDL_UnlockSurface(surface);

    return surface;
}

/* Create the button images for a given label and store them in the
 * button controls image array starting at the given index.
 */
static int makebuttonimages(struct sdlcontrol *ctl, char const *title, int id)
{
    static SDL_Color const textcolor = { 0, 0, 128, 0 };
    static SDL_Color const emcolor = { 0, 0, 191, 0 };
    static SDL_Color const boldcolor = { 0, 0, 255, 0 };
    static SDL_Color const offcolor = { 175, 175, 191, 0 };
    static int const shadowdepth = 72;

    SDL_Surface *face, *image;
    SDL_Color bkgndcolor, outlinecolor;
    SDL_Rect rect;

    if (!font)
	initfont();

    bkgndcolor.r = bkgndcolor.g = bkgndcolor.b = 255 - 2 * shadowdepth / 3;
    outlinecolor.r = outlinecolor.g = outlinecolor.b = 255 - shadowdepth * 2;

    image = makeshadedsurface(buttonwidth, buttonheight, shadowdepth);
    face = SDL_DisplayFormat(image);
    SDL_FreeSurface(image);
    outlinesurface(face, outlinecolor, 1);

    ctl->images[id + s_up] = SDL_DisplayFormat(face);
    image = TTF_RenderUTF8_Blended(font, title, textcolor);
    rect.x = (buttonwidth - image->w) / 2;
    rect.y = (buttonheight - image->h) / 2;
    SDL_BlitSurface(image, NULL, ctl->images[id + s_up], &rect);
    SDL_FreeSurface(image);

    ctl->images[id + s_disabled] = SDL_DisplayFormat(face);
    image = TTF_RenderUTF8_Blended(font, title, offcolor);
    rect.x = (buttonwidth - image->w) / 2;
    rect.y = (buttonheight - image->h) / 2;
    SDL_BlitSurface(image, NULL, ctl->images[id + s_disabled], &rect);
    SDL_FreeSurface(image);

    ctl->images[id + s_over] = SDL_DisplayFormat(face);
    image = TTF_RenderUTF8_Blended(font, title, emcolor);
    rect.x = (buttonwidth - image->w) / 2;
    rect.y = (buttonheight - image->h) / 2;
    SDL_BlitSurface(image, NULL, ctl->images[id + s_over], &rect);
    SDL_FreeSurface(image);

    ctl->images[id + s_down] = SDL_DisplayFormat(face);
    rect.x = 1;
    rect.y = 1;
    rect.w = buttonwidth - 2;
    rect.h = buttonheight - 2;
    SDL_FillRect(ctl->images[id + s_down], &rect,
		 SDL_MapRGB(ctl->images[id + s_down]->format,
			    bkgndcolor.r, bkgndcolor.g, bkgndcolor.b));
    image = TTF_RenderUTF8_Shaded(font, title, boldcolor, bkgndcolor);
    rect.x = (buttonwidth - image->w) / 2;
    rect.y = (buttonheight - image->h) / 2;
    SDL_BlitSurface(image, NULL, ctl->images[id + s_down], &rect);
    SDL_FreeSurface(image);

    SDL_FreeSurface(face);
    return 1;
}

/* Update the control's current state, and return true if the state
 * has changed.
 */
int updatebutton(struct sdlcontrol *ctl)
{
    int state;

    if (isdisabled(*ctl->control))
	state = s_disabled;
    else if (ctl->hovering)
	state = ctl->down ? s_down : s_over;
    else
	state = ctl->down ? s_over : s_up;
    state += ctl->control->value * s_count;
    if (ctl->state == state)
	return 0;
    ctl->state = state;
    return 1;
}

/* Initialize the given control as a button.
 */
int makebutton(struct sdlcontrol *ctl)
{
    int i;

    ctl->images = allocate(bval_count * s_count * sizeof *ctl->images);
    for (i = 0 ; i < bval_count ; ++i)
	makebuttonimages(ctl, titles[i], i * s_count);
    ctl->state = -1;
    return 1;
}
