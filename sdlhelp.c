/* help.c: Displaying online help.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include <ctype.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "yahtzee.h"
#include "gen.h"
#include "iosdlctl.h"

/* Colors for rendering the help text.
 */
static SDL_Color const textcolor = { 191, 191, 255, 0 };
static SDL_Color const bkgndcolor = { 0, 32, 80, 0 };

/* The current font.
 */
static TTF_Font *font;

/* Dimensions of the key graphics.
 */
static int keywidth, keyheight;

/* Wait for an input event. Returns false if the input event was an
 * attempt to exit the program.
 */
static int getkey(void)
{
    SDL_Event event;

    for (;;) {
	if (SDL_WaitEvent(&event) < 0)
	    exit(1);
	if (event.type == SDL_QUIT)
	    return 0;
	else if (event.type == SDL_MOUSEBUTTONDOWN)
	    return 1;
	else if (event.type == SDL_KEYDOWN && event.key.keysym.unicode) {
	    return event.key.keysym.unicode != '\030';
	}
    }
}

/* Load the font and get some dimensions.
 */
static void initfont(char const *path, int fontheight)
{
    font = TTF_OpenFont(path, fontheight);
    if (!font)
	croak("%s\nUnable to load font.", TTF_GetError());
    keyheight = TTF_FontLineSkip(font) + 2;
    keywidth = keyheight + 2;
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

/* Create a set of graphics containing the hot key characters for all
 * of the controls.
 */
static void makekeys(SDL_Surface *keys[], struct sdlcontrol const *sdlcontrols)
{
    static SDL_Color const keytextcolor = { 175, 175, 191, 0 };
    static SDL_Color const keycolor = { 0, 0, 0, 0 };

    SDL_Surface *image;
    SDL_Rect rect;
    int key, i;

    for (i = 0 ; i < ctl_count ; ++i) {
	key = toupper(sdlcontrols[i].control->key);
	if (!key) {
	    keys[i] = NULL;
	    continue;
	}
	if (key == ' ') {
	    image = TTF_RenderUTF8_Shaded(font, "space",
					  keytextcolor, keycolor);
	    keys[i] = SDL_CreateRGBSurface(SDL_SWSURFACE,
					   image->w + keywidth, keyheight,
					   sdl_screen->format->BitsPerPixel,
					   sdl_screen->format->Rmask,
					   sdl_screen->format->Gmask,
					   sdl_screen->format->Bmask,
					   sdl_screen->format->Amask);
	} else {
	    image = TTF_RenderGlyph_Shaded(font, key, keytextcolor, keycolor);
	    keys[i] = SDL_CreateRGBSurface(SDL_SWSURFACE, keywidth, keyheight,
					   sdl_screen->format->BitsPerPixel,
					   sdl_screen->format->Rmask,
					   sdl_screen->format->Gmask,
					   sdl_screen->format->Bmask,
					   sdl_screen->format->Amask);
	}
	SDL_FillRect(keys[i], NULL,
		     SDL_MapRGB(keys[i]->format,
				keycolor.r, keycolor.g, keycolor.b));
	outlinesurface(keys[i], keytextcolor, 1);
	rect.x = (keys[i]->w - image->w) / 2;
	rect.y = (keys[i]->h - image->h) / 2;
	SDL_BlitSurface(image, NULL, keys[i], &rect);
	SDL_FreeSurface(image);
    }
}

/* Render a paragraph of text to the given surface, starting at the
 * given y-coordinate and breaking on whitespace as necessary. Returns
 * the y-coordinate just below the last line rendered.
 */
static int writetext(SDL_Surface *surface, char const *text, int y)
{
    SDL_Surface *image;
    SDL_Rect rect;
    char *buf;
    int textlen, pos, n, w;

    rect.x = keywidth / 2;
    rect.y = y;
    textlen = strlen(text);
    buf = allocate(textlen + 1);
    pos = 0;
    while (pos < textlen) {
	n = textlen - pos;
	memcpy(buf, text + pos, n + 1);
	for (;;) {
	    TTF_SizeUTF8(font, buf, &w, NULL);
	    if (w <= surface->w - keywidth)
		break;
	    while (--n && buf[n] != ' ') ;
	    if (n <= 0) {
		n = strlen(buf);
		break;
	    }
	    buf[n] = '\0';
	}
	image = TTF_RenderUTF8_Shaded(font, buf, textcolor, bkgndcolor);
	SDL_BlitSurface(image, NULL, surface, &rect);
	rect.y += TTF_FontLineSkip(font);
	pos += n + 1;
    }
    return rect.y;
}

/* Display several lines of text (terminated with a null pointer) and
 * wait for a keypress before returning.
 */
static int displaytext(char const *lines[])
{
    int i, y;

    initfont(FONT_MED_PATH, sdl_scalingunit * 3);
    SDL_FillRect(sdl_screen, NULL, 
		 SDL_MapRGB(sdl_screen->format,
			    bkgndcolor.r, bkgndcolor.g, bkgndcolor.b));
    y = TTF_FontLineSkip(font) / 2;
    for (i = 0 ; lines[i] ; ++i)
	y = writetext(sdl_screen, *lines[i] ? lines[i] : " ", y);
    SDL_UpdateRect(sdl_screen, 0, 0, 0, 0);
    TTF_CloseFont(font);
    font = NULL;
    return getkey();
}

/* Temporarily display the online help text.
 */
int runhelp(void)
{
    static char const *helptext[] = {
	"Press Ctrl-+ and Ctrl-\xE2\x80\x93 to resize the window.",
	"Press ? or F1 to view this help text again.",
	"Press Ctrl-K to view the keyboard shortcuts.",
	"Press Ctrl-V to view the version and license information.",
	"Press Ctrl-X to exit the program.",
	NULL
    };

    int i, y;

    initfont(FONT_MED_PATH, sdl_scalingunit * 3);
    SDL_FillRect(sdl_screen, NULL, 
		 SDL_MapRGB(sdl_screen->format,
			    bkgndcolor.r, bkgndcolor.g, bkgndcolor.b));
    y = TTF_FontLineSkip(font) / 2;
    for (i = 0 ; rulesinfo[i] ; ++i)
	y = writetext(sdl_screen, rulesinfo[i], y);
    y += TTF_FontLineSkip(font);
    for (i = 0 ; helptext[i] ; ++i)
	y = writetext(sdl_screen, helptext[i], y);
    SDL_UpdateRect(sdl_screen, 0, 0, 0, 0);
    TTF_CloseFont(font);
    font = NULL;
    return getkey();

    return displaytext(helptext);
}

/* Temporarily display the license text.
 */
int showlicense(void)
{
    int i, y;

    initfont(FONT_MED_PATH, sdl_scalingunit * 3);
    SDL_FillRect(sdl_screen, NULL, 
		 SDL_MapRGB(sdl_screen->format,
			    bkgndcolor.r, bkgndcolor.g, bkgndcolor.b));
    y = TTF_FontLineSkip(font) / 2;
    for (i = 0 ; licenseinfo[i] ; ++i)
	y = writetext(sdl_screen, licenseinfo[i], y);
    SDL_UpdateRect(sdl_screen, 0, 0, 0, 0);
    TTF_CloseFont(font);
    font = NULL;
    return getkey();
}

/* Temporarily render each of the key graphics next to its associated
 * control.
 */
int showkeyhelp(struct sdlcontrol const *sdlcontrols)
{
    SDL_Surface *keys[ctl_count];
    SDL_Rect rect;
    int i;

    initfont(FONT_BOLD_PATH, sdl_scalingunit * 4);
    makekeys(keys, sdlcontrols);
    for (i = 0 ; i < ctl_count ; ++i) {
	if (!keys[i])
	    continue;
	rect.x = sdlcontrols[i].rect.x + 4;
	rect.y = sdlcontrols[i].rect.y - 4;
	SDL_BlitSurface(keys[i], NULL, sdl_screen, &rect);
	SDL_FreeSurface(keys[i]);
    }
    SDL_UpdateRect(sdl_screen, 0, 0, 0, 0);
    TTF_CloseFont(font);
    font = NULL;
    return getkey();
}
