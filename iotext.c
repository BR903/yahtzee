/* iotext.c: The dumb text terminal user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 *
 * NOTE: THIS IS NOT THE REAL CODE FOR THE PLAIN TEXT UI.
 *
 * The original version of this file was lost. This file is only a
 * placeholder, derived from the 1.4 version of the file. It compiles,
 * but the text-mode interface does not function correctly. (The other
 * two interfaces are fine.) Please refer to version 1.4 or later for
 * a real example of this code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "yahtzee.h"
#include "gen.h"
#include "iotext.h"

/* The labels on each of the scoring slots.
 */
static char const *slotnames[ctl_slots_count] = {
    "Ones     ", "Twos     ", "Threes   ", "Fours    ",
    "Fives    ", "Sixes    ", "Subtotal ", "Bonus    ",
    "Three of a Kind ", "Four of a Kind  ",
    "Full House      ", "Small Straight  ",
    "Large Straight  ", "Yahtzee         ",
    "Chance          ", "Total Score     "
};

/* The program keeps a snapshot of the last seen values
 * of the controls so it knows what to display.
 */
static int lastvalues[ctl_count];

/* A small event queue. Needed because a single line of input can map
 * to multiple input events.
 */
static int eventqueue[8];
static unsigned int eventqueuesize = 0;

/* True if the score sheet should be displayed at the next prompt.
 */
static int displayscore;

/* True if the dice should be displayed at the next prompt.
 */
static int displaydice;

/*
 * Using the event queue.
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

/*
 * Display routines.
 */

/* Display the dice on a single line.
 */
static void showdice(void)
{
    int i;

    printf("(%c)", '1' + controls[ctl_dice].value);
    for (i = ctl_dice + 1 ; i < ctl_dice_end ; ++i)
	printf("  (%c)", '1' + controls[i].value);
    printf("\n");
    displaydice = 0;
}

/* Render the complete score sheet in two columns.
 */
static void showscoresheet(void)
{
    int i, m, n;

    m = ctl_slots_count / 2;
    for (n = 0 ; n < m ; ++n) {
	i = ctl_slots + n;
	if (controls[i].key)
	    printf("%c: %s", controls[i].key, slotnames[n]);
	else
	    printf("   %s", slotnames[n]);
	if (controls[i].value >= 0 &&
			(controls[i].disabled || controls[i].set))
	    printf("%4d  .  ", controls[i].value);
	else
	    printf("      .  ");
	i += m;
	if (controls[i].key)
	    printf("%c: %s", controls[i].key, slotnames[m + n]);
	else
	    printf("   %s", slotnames[m + n]);
	if (controls[i].value >= 0 &&
			(controls[i].disabled || controls[i].set))
	    printf("%4d", controls[i].value);
	putchar('\n');
    }
    displayscore = 0;
}

/* Prompt the user based on the current button state.
 */
static void showprompt(void)
{
    int i;

    if (controls[ctl_button].value == bval_newgame) {
	if (displaydice)
	    showdice();
	if (displayscore)
	    showscoresheet();
	printf("Another game (RET) or Quit (q):\n");
	return;
    }

    if (displayscore)
	showscoresheet();
    if (displaydice)
	showdice();
    if (controls[ctl_button].value == bval_score &&
			!controls[ctl_button].disabled) {
	printf("Confirm (RET):\n");
	return;
    }

    if (controls[ctl_button].value == bval_roll)
	printf("Roll (abcde) or ");
    printf("Score (");
    for (i = ctl_slots ; i < ctl_slots_end ; ++i)
	if (!controls[i].disabled)
	    putchar(controls[i].key);
    printf("):\n");
}

/* Display online help.
 */
static int runhelp(void)
{
    printf("%s%s%s%s%s",
	   "   Each turn begins with a roll of the dice. Select which dice\n"
	   "to re-roll by entering the corresponding letters (a), (b), (c),\n"
	   "(d), and/or (e). Two re-rolls are permitted per turn. After\n"
	   "re-rolling, choose where to score the dice by entering the\n"
	   "letter or number for that line on the score sheet.\n",
	   "   A full house is worth 25 points, a small straight (four\n"
	   "dice) scores 30 points, a large straight (five dice) scores 40\n"
	   "points, and yahtzee scores 50 points. Three of a kind, four of\n"
	   "a kind, and chance score the total of all five dice.\n",
	   "   If the left side scores 63 or more points, a bonus of 35\n"
	   "points is awarded.\n",
	   "   At any time you can type (q) to exit the program, (.) to\n"
	   "re-display the game state, (v) to see the version information,\n"
	   "or (?) to view this help text again.\n",
	   "Press (RET) to return to the game.\n");
    for (;;) {
	switch (getchar()) {
	  case '\n':
	    return 1;
	  case 'Q':
	  case 'q':
	  case EOF:
	    return 0;
	}
    }
}

/* Display version information.
 */
static int showversion(void)
{
    int i;

    for (i = 0 ; versioninfo[i] ; ++i)
	printf("   %s\n", versioninfo[i]);
    printf("Press (RET) to return to the game.\n");
    for (;;) {
	switch (getchar()) {
	  case '\n':
	    return 1;
	  case 'Q':
	  case 'q':
	  case EOF:
	    return 0;
	}
    }
}

/* Process a line of user input. Determine if the input corresponds to
 * the dice, the slots, or the button. Returns zero if the input was
 * invalid.
 */
static int processinput(char const *str, int length)
{
    char dice[ctl_dice_count];
    char const *p;
    int i, n;

    if (length == 0) {
	if (controls[ctl_button].disabled) {
	    printf("Enter (?) for help.\n");
	    return 0;
	}
	queueinputevent(ctl_button);
	return 1;
    }
    if (controls[ctl_button].value == bval_newgame)
	return 0;

    n = 0;
    memset(dice, 0, sizeof dice);
    for (p = str ; *p ; ++p) {
	for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	    if (controls[i].key == tolower(*p)) {
		if (controls[i].disabled) {
		    printf("Cannot roll dice.\n");
		    return 0;
		}
		dice[i - ctl_dice] = 1;
		++n;
		break;
	    }
	}
    }
    if (n) {
	if (n == length) {
	    for (i = 0 ; i < ctl_dice_count ; ++i)
		if (dice[i])
		    queueinputevent(ctl_dice + i);
	    queueinputevent(ctl_button);
	    return 1;
	} else {
	    printf("Please specify either dice or a single scoring slot.\n");
	    return 0;
	}
    }

    for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
	if (controls[i].key == tolower(*str)) {
	    if (controls[i].disabled) {
		printf("Slot not available.\n");
		return 0;
	    }
	    if (n > 1) {
		printf("Extra characters after input: \"%s\".\n", str + 1);
		return 0;
	    }
	    queueinputevent(i);
	    return 1;
	}
    }

    printf("Invalid input. Enter (?) for help.\n");
    return 0;
}

/*
 * Exported functions.
 */

/* Initialize the internal state.
 */
int text_initializeio(void)
{
    int i;

    printf("\nY a h t z e e\n\n");
    for (i = ctl_slots ; i < ctl_slots_end ; ++i)
	lastvalues[i] = controls[i].value;
    displayscore = 0;
    displaydice = 1;
    return 1;
}

/* A placeholder; rendering is actually done in text_getinputevent().
 */
void text_render(void)
{
    return;
}

/* Display changes to the state and return the next input event from
 * the queue.
 */
int text_getinputevent(int *control, int *action)
{
    char buf[8];
    int ch, i, n;

    for (;;) {
	if (unqueueinputevent(control)) {
	    *action = act_clicked;
	    displaydice = 1;
	    return 1;
	}
	for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
	    n = controls[i].disabled || controls[i].set;
	    if (lastvalues[i] != n) {
		lastvalues[i] = n;
		displayscore = 1;
	    }
	}

	showprompt();
	if (!fgets(buf, sizeof buf, stdin)) {
	    if (ferror(stdin))
		exit(1);
	    return 0;
	}
	n = strlen(buf);
	if (n == 0 || tolower(*buf) == 'q')
	    return 0;
	if (n == sizeof buf - 1 && buf[n - 1] != '\n')
	    for (ch = getchar() ; ch != '\n' && ch != EOF ; ch = getchar()) ;
	else
	    buf[--n] = '\0';
	if (*buf == '.') {
	    displayscore = 1;
	    displaydice = 1;
	    continue;
	}
	if (*buf == '?') {
	    if (!runhelp())
		return 0;
	    continue;
	}
	if (tolower(*buf) == 'v') {
	    if (!showversion())
		return 0;
	    continue;
	}
	if (!processinput(buf, n))
	    putchar('\a');
    }
}
