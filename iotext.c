/* iotext.c: The dumb text terminal user interface.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
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
			(isdisabled(controls[i]) || isselected(controls[i])))
	    printf("%4d  .  ", controls[i].value);
	else
	    printf("      .  ");
	i += m;
	if (controls[i].key)
	    printf("%c: %s", controls[i].key, slotnames[m + n]);
	else
	    printf("   %s", slotnames[m + n]);
	if (controls[i].value >= 0 &&
			(isdisabled(controls[i]) || isselected(controls[i])))
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

    for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	if (ismodified(controls[i])) {
	    clearmodified(controls[i]);
	    displaydice = 1;
	}
    }

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
			!isdisabled(controls[ctl_button])) {
	printf("Confirm (RET):\n");
	return;
    }

    if (controls[ctl_button].value == bval_roll)
	printf("Roll (abcde) or ");
    printf("Score (");
    for (i = ctl_slots ; i < ctl_slots_end ; ++i)
	if (!isdisabled(controls[i]))
	    putchar(controls[i].key);
    printf("):\n");
}

/* Display online help.
 */
static void showhelp(void)
{
    char const *line;
    int i, n;

    for (i = 0 ; rulesinfo[i] ; ++i) {
	line = rulesinfo[i];
	while (*line) {
	    n = textbreak(&line, 72);
	    printf("%.*s\n", n, line);
	    line += n;
	}
    }
    printf("\n%s",
	   "At any time you can type (q) to exit the program, (.) to\n"
	   "re-display the game state, (v) to see the version and license\n"
	   "information, or (?) to view this help text again.\n");
}

/* Display version and license information.
 */
static void showversion(void)
{
    char const *line;
    int i, n;

    for (i = 0 ; licenseinfo[i] ; ++i) {
	line = licenseinfo[i];
	while (*line) {
	    n = textbreak(&line, 64);
	    printf("%.*s\n", n, line);
	    line += n;
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
	if (isdisabled(controls[ctl_button])) {
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
		if (isdisabled(controls[i])) {
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
	    if (isdisabled(controls[i])) {
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
    displaydice = 0;
    return 1;
}

/* Display changes to the state and return the next input event from
 * the queue.
 */
int text_runio(int *control)
{
    char buf[8];
    int ch, i, n;

    for (;;) {
	if (unqueueinputevent(control))
	    return 1;
	for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
	    n = isdisabled(controls[i]) || isselected(controls[i]);
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
	    showhelp();
	    displayscore = 0;
	    displaydice = 0;
	    continue;
	}
	if (tolower(*buf) == 'v') {
	    showversion();
	    displayscore = 0;
	    displaydice = 0;
	    continue;
	}
	if (!processinput(buf, n))
	    fputc('\a', stderr);
    }
}
