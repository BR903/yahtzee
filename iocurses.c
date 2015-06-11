/* iocurses.c: The curses user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include "yahtzee.h"
#include "gen.h"
#include "iocurses.h"

/* Shorthand macro for changing the current text attributes.
 */
#define aset(a) ((void)attrset(attrs[a]))

/* The list of attribute types used.
 */
enum { a_normal, a_dim, a_marked, a_selected, a_overlay, a_count };

/* The sizes of the various controls on the display.
 */
static int const cxDie = 9, cyDie = 5, cxDieSpacing = 2;
static int const cxDice = 9 * 5 + 2 * 4;
static int const cxButton = 13;
static int const cxSlot = 20, cySlots = 8, cxSlotSpacing = 7;
static int const cxSlots = 20 * 2 + 7;

/* The layouts for the ASCII-art die faces.
 */
static char const *diepatterns[] = {
    "422222225422222225422222225422222225422222225422222225",
    "300000003300000103300000103301000103301000103301000103",
    "300010003300000003300010003300000003300010003301000103",
    "300000003301000003301000003301000103301000103301000103",
    "622222227622222227622222227622222227622222227622222227"
};

/* The placement of the various controls on the display.
 */
static int xDice, yDice;
static int xButton, yButton;
static int xSlots, ySlots;

/* The list of text attribute settings.
 */
static chtype attrs[a_count];

/* The size of the display.
 */
static int cxScreen, cyScreen;

/* The actual set of characters used to render the die faces.
 */
static chtype diechars[8];

/*
 * The I/O functions.
 */

/* Called on program exit.
 */
static void shutdown(void)
{
    if (!isendwin())
	endwin();
}

/* Calculate the locations of the screen elements.
 */
static void initlayout(void)
{
    getmaxyx(stdscr, cyScreen, cxScreen);
    xDice = (cxScreen - cxDice) / 2;
    if (xDice < 1)
	xDice = 1;
    xButton = xDice + (cxDice - cxButton) / 2;
    xSlots = xDice + (cxDice - cxSlots) / 2;
    yDice = 0;
    yButton = yDice + cyDie + 1;
    ySlots = yButton + 2;
}

/* Select a set of characters for rendering dice. There are three
 * possible renderings. The first, which is the default, uses only
 * standard ASCII characters. In the second set, VT100 line-drawing
 * characters replace the dashes and pipe charaters. The last set also
 * uses the VT100 bullet character to draw the pips.
 */
static void changediechars(int advance)
{
    static int which = 0;

    which = (which + advance) % 3;
    switch (which) {
      case 0:
	diechars[0] = ' ';		diechars[4] = ' ';
	diechars[1] = 'o';		diechars[5] = ' ';
	diechars[2] = '-';		diechars[6] = ' ';
	diechars[3] = '|';		diechars[7] = ' ';
	break;
      case 1:
	diechars[0] = ' ';		diechars[4] = ACS_ULCORNER;
	diechars[1] = 'o';		diechars[5] = ACS_URCORNER;
	diechars[2] = ACS_HLINE;	diechars[6] = ACS_LLCORNER;
	diechars[3] = ACS_VLINE;	diechars[7] = ACS_LRCORNER;
	break;
      case 2:
	diechars[0] = ' ';		diechars[4] = ACS_ULCORNER;
	diechars[1] = ACS_BULLET;	diechars[5] = ACS_URCORNER;
	diechars[2] = ACS_HLINE;	diechars[6] = ACS_LLCORNER;
	diechars[3] = ACS_VLINE;	diechars[7] = ACS_LRCORNER;
	break;
    }
}

/* Render a die using the current set of character graphics.
 */
static void drawacsdie(int y, int x, int v)
{
    int i, j;

    for (j = 0 ; j < cyDie ; ++j) {
	move(y + j, x);
	for (i = 0 ; i < cxDie ; ++i)
	    addch(diechars[diepatterns[j][v * cxDie + i] - '0']);
    }
}

/* Update the display.
 */
static void render(void)
{
    static char const *buttontext[bval_count] = { 
	"Roll Dice", "  Score  ", "  Again  "
    };
    static char const *slottext[ctl_slots_count] = {
	"Ones", "Twos", "Threes", "Fours", "Fives", "Sixes", "Subtotal",
	"Bonus", "Three of a Kind", "Four of a Kind", "Full House",
	"Small Straight", "Large Straight", "Yahtzee", "Chance", "Total Score"
    };

    char const *text;
    int i, n, x;

    erase();

    /* The dice. */

    x = xDice;
    for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	if (isselected(controls[i]))
	    aset(a_marked);
	drawacsdie(yDice, x, controls[i].value);
	if (isselected(controls[i]))
	    aset(a_normal);
	x += cxDie + cxDieSpacing;
    }

    /* The button. */

    text = buttontext[controls[ctl_button].value];
    move(yButton, xButton);
    if (isdisabled(controls[ctl_button])) {
	aset(a_dim);
	printw("| %s |", text);
	aset(a_normal);
    } else if (isselected(controls[ctl_button])) {
	addstr("[[");
	aset(a_selected);
	addstr(text);
	aset(a_normal);
	addstr("]]");
    } else {
	printw("[ %s ]", text);
    }

    /* The scoring slots. */

    i = ctl_slots;
    x = xSlots;
    while (i < ctl_slots_end) {
	for (n = 0 ; n < ctl_slots_count / 2 ; ++n, ++i) {
	    move(ySlots + n, x);
	    if (isselected(controls[i]))
		aset(a_selected);
	    printw("%-*s", cxSlot - 3, slottext[i - ctl_slots]);
	    if (controls[i].value >= 0) {
		if (isdisabled(controls[i]) || isselected(controls[i]))
		    printw("%3d", controls[i].value);
	    }
	    aset(a_normal);
	}
	x += cxSlot + cxSlotSpacing;
    }

    move(cyScreen - 1, 0);
    refresh();
}

/* Temporarily display some text and wait for a keypress.
 */
static int runtextdisplay(char const *title, char const *lines[])
{
    char const *line;
    int y, i, n;

    erase();
    mvaddstr(0, xDice + 4, title);
    y = 2;
    for (i = 0 ; lines[i] ; ++i) {
	line = lines[i];
	while (*line) {
	    n = textbreak(&line, 72);
	    mvaddnstr(y, 4, line, n);
	    ++y;
	    line += n;
	}
    }
    move(cyScreen - 1, 0);
    refresh();

    for (;;) {
	switch (getch()) {
	  case '\003':
	  case '\030':
	    return 0;
	  default:
	    render();
	    return 1;
	}
    }
}

/* Temporarily display text describing the rules of the game and wait
 * for a keypress.
 */
static int runruleshelp(void)
{
    return runtextdisplay("Rules of the Game", rulesinfo);
}

/* Temporarily display the license and wait for a keypress.
 */
static int runlicensedisplay(void)
{
    return runtextdisplay("Version and License", licenseinfo);
}

/* Temporarily overlay text describing the key commands and wait for a
 * keypress.
 */
static int runhelp(void)
{
    int i, n, x, y;

    aset(a_overlay);
    mvaddstr(yDice, xDice - 1, "Use these letter keys to select the dice:");
    x = xDice + cxDie;
    for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	mvprintw(yDice + cyDie - 2, x - 3, "(%c)", toupper(controls[i].key));
	x += cxDie + cxDieSpacing;
    }
    mvaddstr(yButton - 1, xDice - 1, "Use (space) or (return)");
    mvaddstr(yButton, xDice + 1, "to push the button:");
    mvaddstr(ySlots - 1, xDice - 1,
	     "Use these keys to select a scoring slot:");
    i = ctl_slots;
    x = xSlots;
    while (i < ctl_slots_end) {
	for (n = 0 ; n < ctl_slots_count / 2 ; ++n, ++i)
	    if (controls[i].key)
		mvprintw(ySlots + n, x - 4, "(%c)", toupper(controls[i].key));
	x += cxSlot + cxSlotSpacing;
    }
    y = ySlots + cySlots;
    mvaddstr(y + 0, xDice - 1, "(ctrl A) changes how the dice are drawn.");
    mvaddstr(y + 1, xDice - 1, "Use (?) or (F1) to view this help again.");
    mvaddstr(y + 2, xDice - 1, "Use (ctrl R) to view the rules.");
    mvaddstr(y + 3, xDice - 1,
	     "Use (ctrl V) to see version and license information.");
    mvaddstr(y + 5, xDice + cxDice - 33, "Use (ctrl X) to exit the program.");
    aset(a_normal);
    move(cyScreen - 1, 0);
    refresh();

    for (;;) {
	switch (getch()) {
	  case '\003':
	  case '\030':
	    return 0;
	  case '\022':
	    return runruleshelp();
	    break;
	  case '\026':
	    return runlicensedisplay();
	    break;
	  default:
	    render();
	    return 1;
	}
    }
}

/* Determine if a mouse click occurred over one of the controls.
 */
static int getmouseevent(int *control)
{
    MEVENT event;
    int i, x;

    if (getmouse(&event) != OK || event.bstate != BUTTON1_CLICKED)
	return 0;
    if (event.y >= yDice && event.y < yDice + cyDie) {
	x = xDice;
	for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	    if (event.x >= x && event.x < x + cxDie) {
		*control = i;
		return 1;
	    }
	    x += cxDie + cxDieSpacing;
	}
    }
    if (event.y == yButton && event.x >= xButton
			   && event.x < xButton + cxButton) {
	*control = ctl_button;
	return 1;
    }
    if (event.y >= ySlots && event.y < ySlots + cySlots) {
	if (event.x >= xSlots && event.x < xSlots + cxSlot) {
	    *control = ctl_slots + (event.y - ySlots);
	    return 1;
	}
	if (event.x >= xSlots + cxSlot + cxSlotSpacing &&
			event.x < xSlots + cxSlot * 2 + cxSlotSpacing) {
	    *control = ctl_slots + (ctl_slots_count / 2) + (event.y - ySlots);
	    return 1;
	}
    }
    return 0;
}

/*
 * Exported functions.
 */

/* Initialize the curses subsystem and register a palette of colors.
 */
int curses_initializeio(void)
{
    if (!initscr())
	return 0;
    atexit(shutdown);
    initlayout();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(BUTTON1_CLICKED, NULL);
    if (has_colors()) {
	start_color();
	init_pair(1, COLOR_BLACK, 0);
	init_pair(2, COLOR_CYAN, 0);
	init_pair(3, COLOR_YELLOW, 0);
	init_pair(4, COLOR_BLUE, 0);
	attrs[a_normal] = A_NORMAL;
	attrs[a_dim] = COLOR_PAIR(1) | A_BOLD;
	attrs[a_marked] = COLOR_PAIR(2);
	attrs[a_selected] = COLOR_PAIR(3);
	attrs[a_overlay] = COLOR_PAIR(4) | A_BOLD;
    } else {
	attrs[a_normal] = A_NORMAL;
	attrs[a_dim] = A_DIM;
	attrs[a_marked] = A_DIM;
	attrs[a_selected] = A_STANDOUT;
	attrs[a_overlay] = A_BOLD;
    }
    changediechars(0);
    return 1;
}

/* Update the display and wait for an input event.
 */
int curses_runio(int *control)
{
    int ch, i;

    for (;;) {
	render();
	ch = tolower(getch());
	switch (ch) {
	  case '\r':
	  case '\n':
	  case KEY_ENTER:
	    *control = ctl_button;
	    return 1;
	  case KEY_MOUSE:
	    if (getmouseevent(control))
		return 1;
	    break;
	  case '?':
	  case KEY_F(1):
	    if (!runhelp())
		return 0;
	    break;
	  case '\022':
	    if (!runruleshelp())
		return 0;
	    break;
	  case '\026':
	    if (!runlicensedisplay())
		return 0;
	    break;
	  case '\001':
	    changediechars(+1);
	    break;
	  case '\f':
	    clearok(stdscr, TRUE);
	    break;
	  case KEY_RESIZE:
	    initlayout();
	    break;
	  case '\003':
	  case '\030':
	    return 0;
	  case 'q':
	  case '\033':
	    if (controls[ctl_button].value == bval_newgame)
		return 0;
	    break;
	  case ERR:
	    exit(1);
	  default:
	    for (i = 0 ; i < ctl_count ; ++i) {
		if (controls[i].key == ch) {
		    *control = i;
		    return 1;
		}
	    }
	}
    }
}
