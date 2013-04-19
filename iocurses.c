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

/* The ASCII-graphic renderings of the die faces.
 */
static char const *dielines[5][6] = {
    {" ------- "," ------- "," ------- "," ------- "," ------- "," ------- "},
    {"|       |","|     o |","|     o |","| o   o |","| o   o |","| o   o |"},
    {"|   o   |","|       |","|   o   |","|       |","|   o   |","| o   o |"},
    {"|       |","| o     |","| o     |","| o   o |","| o   o |","| o   o |"},
    {" ------- "," ------- "," ------- "," ------- "," ------- "," ------- "}
};

/* The text of the button labels.
 */
static char const *buttontext[bval_count] = { 
    "Roll Dice", "  Score  ", "Try Again"
};

/* The labels on each of the scoring slots.
 */
static char const *slottext[ctl_slots_count] = {
    "Ones", "Twos", "Threes", "Fours", "Fives", "Sixes", "Subtotal",
    "Bonus", "Three of a Kind", "Four of a Kind", "Full House",
    "Small Straight", "Large Straight", "Yahtzee", "Chance", "Total Score"
};

/* The sizes of the various controls on the display.
 */
static int const cxDie = 9, cyDie = 5, cxDieSpacing = 2;
static int const cxDice = 9 * 5 + 2 * 4;
static int const cxButton = 13;
static int const cxSlot = 20, cySlots = 8, cxSlotSpacing = 7;
static int const cxSlots = 20 * 2 + 7;

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

/* Update the display.
 */
static void render(void)
{
    char const *text;
    int i, n, x;

    erase();

    /* The dice. */

    x = xDice;
    for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	if (isselected(controls[i]))
	    aset(a_marked);
	for (n = 0 ; n < cyDie ; ++n)
	    mvaddstr(yDice + n, x, dielines[n][controls[i].value]);
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

/* Temporarily display text describing the rules of the game and wait
 * for a keypress.
 */
static int runruleshelp(void)
{
    static char const *helptext[] = {
	"Each turn begins with a roll of the dice. Select which dice to",
	"re-roll and push the Roll Dice button. Two re-rolls are",
	"permitted per turn. After re-rolling, choose where to score",
	"the dice and push the Score button.",
	"",
	"A full house is worth 25 points, a small straight (four dice)",
	"scores 30 points, a large straight (five dice) scores 40",
	"points, and yahtzee scores 50 points. Three of a kind, four of",
	"a kind, and chance score the total of all five dice.",
	"",
	"If the left side scores 63 or more points, a bonus of 35",
	"points is awarded.",
	"",
	NULL
    };

    int i;

    erase();
    mvaddstr(0, xDice + 4, "Rules of the Game");
    for (i = 0 ; helptext[i] ; ++i)
	mvaddstr(2 + i, xDice - 1, helptext[i]);

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
    mvaddstr(yButton - 1, xDice + cxDice - 11, "Use (ctrl X)");
    mvaddstr(yButton, xDice + cxDice - 9, "to exit.");
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
    mvaddstr(y, xDice - 1, "Use (?) or (F1) to view this help again.");
    mvaddstr(y + 1, xDice - 1, "Use (ctrl R) to view the rules.");
    for (i = 0 ; versioninfo[i] ; ++i)
	mvaddstr(y + i + 3, xDice - 1, versioninfo[i]);
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
	  case '\f':
	    clearok(stdscr, TRUE);
	    break;
	  case KEY_RESIZE:
	    initlayout();
	    break;
	  case '\003':
	  case '\030':
	    return 0;
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
