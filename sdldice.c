/* dice.c: An SDL die control.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include <math.h>
#include "SDL.h"
#include "yahtzee.h"
#include "gen.h"
#include "iosdlctl.h"

/* Images for all dice controls. The first six images are for each
 * face; the other six show the same faces but shaded to indicate
 * selection.
 */
static SDL_Surface *dieimages[12];
static int diesize;

/* Set a single pixel in a 32-bit surface.
 */
#define setpixel(s, x, y, p) \
    (((Uint32*)(((Uint8*)((s)->pixels)) + (y) * (s)->pitch))[x] = (p))

/* Transform cartesian coordinates to polar coordinates.
 */
static void topolar(int x, int y, float *r, float *t)
{
    *r = sqrtf(x * x + y * y);
    *t = atanf((float)y / x);
}

/* Draw a rectangle with a white fill, a one-pixel black border, and
 * rounded corners of the given radius.
 */
static void drawroundedrect(SDL_Surface *image, int radius)
{
    float r, t;
    Uint32 n;
    int x, y;

    SDL_FillRect(image, NULL, 0xFFFFFFFF);
    if (SDL_MUSTLOCK(image))
	SDL_LockSurface(image);
    for (y = radius + 1 ; y < diesize - radius - 1 ; ++y) {
	setpixel(image, 0, y, 0xFF000000);
	setpixel(image, diesize - 1, y, 0xFF000000);
    }
    for (x = radius + 1 ; x < diesize - radius - 1 ; ++x) {
	setpixel(image, x, 0, 0xFF000000);
	setpixel(image, x, diesize - 1, 0xFF000000);
    }
    for (y = 0 ; y <= radius ; ++y) {
	for (x = 0 ; x <= radius ; ++x) {
	    topolar(x, y, &r, &t);
	    if (r < radius - 1) {
		n = 0xFFFFFFFF;
	    } else if (r < radius) {
		n = (int)(255.0 * (radius - r)) & 255;
		n = 0xFF000000 | (n << 16) | (n << 8) | n;
	    } else if (r < radius + 1) {
		n = (int)(255.0 * (r - radius)) & 255;
		n = (255 - n) << 24;
	    } else {
		n = 0x00000000;
	    }
	    setpixel(image, radius - x, radius - y, n);
	    setpixel(image, diesize - 1 - radius + x, radius - y, n);
	    setpixel(image, diesize - 1 - radius + x,
			    diesize - 1 - radius + y, n);
	    setpixel(image, radius - x, diesize - 1 - radius + y, n);
	}
    }
    if (SDL_MUSTLOCK(image))
	SDL_UnlockSurface(image);
}

/* Create the image of a pip with the given radius.
 */
static Uint8 *makepip(int width, int height, int radius)
{
    Uint8 *pixels, *p;
    float r, t, c;
    int xcenter, ycenter;
    int x, y;

    xcenter = width / 2;
    ycenter = height / 2;
    p = pixels = allocate(width * height);
    for (y = 0 ; y < height ; ++y) {
	for (x = 0 ; x < width ; ++x) {
	    topolar(x - xcenter, ycenter - y, &r, &t);
	    if (r - 0.5 > radius) {
		*p++ = 255;
		continue;
	    }
	    c = 0.0;
	    if (t < 0 && x > xcenter && y > ycenter) {
		c += 64.0 * sinf(-t * 2);
		if (r < 4.0)
		    c *= r / 4.0;
	    }
	    if (r + 0.5 > radius)
		c += (240.0 - c) * (r + 0.5 - radius);
	    *p++ = (int)c & 255;
	}
    }
    return pixels;
}

/* Copy a pip image onto a surface at the given location. Noise is
 * introduced so that the copies aren't completely identical.
 */
static void copypip(SDL_Surface *image, int xpos, int ypos,
		    Uint8 const *pip, int width, int height)
{
    Uint8 const *p;
    int x, y, n;

    if (SDL_MUSTLOCK(image))
	SDL_LockSurface(image);
    p = pip;
    for (y = 0 ; y < height ; ++y) {
	for (x = 0 ; x < width ; ++x) {
	    n = *p++;
	    if (n <= 240)
		n += (int)((16.0 * rand()) / (RAND_MAX + 1.0));
	    setpixel(image, xpos + x, ypos + y,
		     0xFF000000 | (n << 16) | (n << 8) | n);
	}
    }
    if (SDL_MUSTLOCK(image))
	SDL_UnlockSurface(image);
}

/* Create the 12 faces (6 selected, 6 unselected) of a die control.
 */
static void renderdieimages(void)
{
    static unsigned char const pippos[6][6][2] = {
	{ { 1, 1 }, { 9, 9 }, { 9, 9 }, { 9, 9 }, { 9, 9 }, { 9, 9 } },
	{ { 2, 0 }, { 0, 2 }, { 9, 9 }, { 9, 9 }, { 9, 9 }, { 9, 9 } },
	{ { 2, 0 }, { 1, 1 }, { 0, 2 }, { 9, 9 }, { 9, 9 }, { 9, 9 } },
	{ { 0, 0 }, { 2, 0 }, { 0, 2 }, { 2, 2 }, { 9, 9 }, { 9, 9 } },
	{ { 0, 0 }, { 2, 0 }, { 1, 1 }, { 0, 2 }, { 2, 2 }, { 9, 9 } },
	{ { 0, 0 }, { 2, 0 }, { 0, 1 }, { 2, 1 }, { 0, 2 }, { 2, 2 } },
    };

    SDL_Surface *grayness, *image;
    SDL_Rect rect;
    Uint8 *pip;
    int pos[3];
    int radius, pipsize;
    int i, j;

    diesize = sdl_scalingunit * 16;
    radius = sdl_scalingunit * 2 - 1;
    image = SDL_CreateRGBSurface(SDL_SWSURFACE, diesize, diesize, 32,
				 0x000000FF, 0x0000FF00,
				 0x00FF0000, 0xFF000000);
    drawroundedrect(image, radius + 1);

    grayness = SDL_CreateRGBSurface(SDL_SWSURFACE, diesize, diesize, 32,
				    0x000000FF, 0x0000FF00,
				    0x00FF0000, 0xFF000000);
    SDL_FillRect(grayness, NULL, 0x7F7F7F7F);

    pipsize = radius * 2 + 3;
    pip = makepip(pipsize, pipsize, radius);

    pos[0] = 3 * sdl_scalingunit - pipsize / 2;
    pos[1] = 8 * sdl_scalingunit - 1 - pipsize / 2;
    pos[2] = 13 * sdl_scalingunit - 2 - pipsize / 2;
    rect.x = radius;
    rect.y = radius;
    rect.w = diesize - radius * 2;
    rect.h = diesize - radius * 2;

    for (i = 0 ; i < 6 ; ++i) {
	SDL_FillRect(image, &rect, 0xFFFFFFFF);
	for (j = 0 ; j <= i ; ++j)
	    copypip(image, pos[pippos[i][j][0]], pos[pippos[i][j][1]],
		    pip, pipsize, pipsize);
	dieimages[i] = SDL_DisplayFormatAlpha(image);
	dieimages[i + 6] = SDL_DisplayFormatAlpha(image);
	SDL_BlitSurface(grayness, NULL, dieimages[i + 6], NULL);
    }
    free(pip);
    SDL_FreeSurface(image);
    SDL_FreeSurface(grayness);
}

/* Update the control's current state, and return true if the state
 * has changed.
 */
int updatedie(struct sdlcontrol *ctl)
{
    int state;

    state = ctl->control->value;
    if (ctl->control->set)
	state += 6;
    if (ctl->state == state)
	return 0;
    ctl->state = state;
    return 1;
}

/* Initialize the given control as a die.
 */
int makedie(struct sdlcontrol *ctl)
{
    if (!dieimages[0])
	renderdieimages();
    ctl->images = dieimages;
    ctl->state = -1;
    return 1;
}