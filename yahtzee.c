/* yahtzee.c: The game logic.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "yahtzee.h"
#include "gen.h"
#include "scoring.h"
#include "io.h"

/* Macros for changing the control flags.
 */
#define setselected(control)	((control).flags |= ctlflag_selected)
#define clearselected(control)	((control).flags &= ~ctlflag_selected)
#define flipselected(control)	((control).flags ^= ctlflag_selected)
#define setdisabled(control)	((control).flags |= ctlflag_disabled)
#define cleardisabled(control)	((control).flags &= ~ctlflag_disabled)
#define setmodified(control)	((control).flags |= ctlflag_modified)

/* The array of I/O controls.
 */
struct control controls[ctl_count];

/*
 * I/O control management.
 */

/* Put the controls in their initial states and set their hotkeys.
 */
static void initcontrols(void)
{
    int i;

    for (i = 0 ; i < ctl_count ; ++i) {
	controls[i].value = -1;
	cleardisabled(controls[i]);
	clearselected(controls[i]);
	clearmodified(controls[i]);
    }

    controls[ctl_die_0].key = 'a';
    controls[ctl_die_1].key = 'b';
    controls[ctl_die_2].key = 'c';
    controls[ctl_die_3].key = 'd';
    controls[ctl_die_4].key = 'e';
    controls[ctl_button].key = ' ';
    controls[ctl_slot_ones].key = '1';
    controls[ctl_slot_twos].key = '2';
    controls[ctl_slot_threes].key = '3';
    controls[ctl_slot_fours].key = '4';
    controls[ctl_slot_fives].key = '5';
    controls[ctl_slot_sixes].key = '6';
    controls[ctl_slot_threeofakind].key = 't';
    controls[ctl_slot_fourofakind].key = 'f';
    controls[ctl_slot_fullhouse].key = 'h';
    controls[ctl_slot_smallstraight].key = 's';
    controls[ctl_slot_largestraight].key = 'l';
    controls[ctl_slot_yahtzee].key = 'y';
    controls[ctl_slot_chance].key = 'x';
}

/* Unmark and roll all of the dice.
 */
static void rollalldice(void)
{
    int i;

    for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	cleardisabled(controls[i]);
	clearselected(controls[i]);
	controls[i].value = (int)((rand() * 6.0) / (double)RAND_MAX);
	setmodified(controls[i]);
    }
    updateopenslots();
}

/* Reroll the dice that are currently selected.
 */
static void rolldice(void)
{
    int i;

    for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	if (isselected(controls[i])) {
	    clearselected(controls[i]);
	    controls[i].value = (int)((rand() * 6.0) / (double)RAND_MAX);
	    setmodified(controls[i]);
	}
    }
    updateopenslots();
}

/* Mark the dice as fixed.
 */
static void freezedice(void)
{
    int i;

    for (i = ctl_dice ; i < ctl_dice_end ; ++i)
	setdisabled(controls[i]);
}

/* Erase the scores in all of the slots.
 */
static void clearallslots(void)
{
    int i;

    for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
	controls[i].value = -1;
	clearselected(controls[i]);
	if (controls[i].key)
	    cleardisabled(controls[i]);
	else
	    setdisabled(controls[i]);
    }
}

/*
 * The main loop.
 */

/* Run a single session of the game. Returns true if the game ran to
 * completion, or false if the user quit. This function embodies the
 * game's state machine. Each iteration of the main loop has three
 * stages: one, update the controls to indicate the game's current
 * state; two, retrieve input from the user; and three, apply the
 * user's action to the game state.
 */
static int playgame(void)
{
    struct control *control;
    struct control *selectedslot;
    int slotopencount, rollcount;
    int ctl, i;

    clearallslots();
    rollalldice();
    rollcount = 1;

    for (;;) {
	selectedslot = 0;
	slotopencount = 0;
	for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
	    if (!isdisabled(controls[i]))
		++slotopencount;
	    if (isselected(controls[i]))
		selectedslot = &controls[i];
	}
	if (slotopencount == 0) {
	    updatescores();
	    break;
	}
	if (rollcount == 3 || selectedslot) {
	    controls[ctl_button].value = bval_score;
	    if (selectedslot)
		cleardisabled(controls[ctl_button]);
	    else
		setdisabled(controls[ctl_button]);
	    if (rollcount == 3)
		freezedice();
	} else {
	    controls[ctl_button].value = bval_roll;
	    setdisabled(controls[ctl_button]);
	    for (i = ctl_dice ; i < ctl_dice_end ; ++i)
		if (isselected(controls[i]))
		    cleardisabled(controls[ctl_button]);
	}

	getnextevent:
	if (rollcount == 3 && slotopencount == 1) {
	    if (selectedslot) {
		ctl = ctl_button;
	    } else {
		for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
		    if (!isdisabled(controls[i])) {
			ctl = i;
			break;
		    }
		}
	    }
	} else {
	    if (!runio(&ctl))
		return 0;
	}
	control = &controls[ctl];
	if (isdisabled(*control))
	    goto getnextevent;

	if (ctl >= ctl_dice && ctl < ctl_dice_end) {
	    if (rollcount == 3)
		goto getnextevent;
	    flipselected(*control);
	    if (selectedslot) {
		clearselected(*selectedslot);
		setmodified(*selectedslot);
		updatescores();
	    }
	    continue;
	}
	if (ctl >= ctl_slots && ctl < ctl_slots_end) {
	    if (isdisabled(*control))
		goto getnextevent;
	    setselected(*control);
	    setmodified(*control);
	    if (selectedslot) {
		clearselected(*selectedslot);
		setmodified(*selectedslot);
	    }
	    for (i = ctl_dice ; i < ctl_dice_end ; ++i)
		clearselected(controls[i]);
	    updatescores();
	    continue;
	}
	if (ctl == ctl_button) {
	    if (control->value == bval_score) {
		if (!selectedslot) {
		    setdisabled(*control);
		    goto getnextevent;
		}
		setdisabled(*selectedslot);
		clearselected(*selectedslot);
		setmodified(*selectedslot);
		if (slotopencount > 1) {
		    rollalldice();
		    rollcount = 1;
		}
	    } else {
		if (rollcount == 3) {
		    setdisabled(*control);
		    goto getnextevent;
		}
		rolldice();
		++rollcount;
	    }
	    continue;
	}
    }

    return 1;
}

/* Handle I/O inbetween games. Returns true if the user asked to start
 * another session. The button is the only active control.
 */
static int newgame(void)
{
    int ctl;

    controls[ctl_button].value = bval_newgame;
    cleardisabled(controls[ctl_button]);
    for (;;) {
	if (!runio(&ctl))
	    return 0;
	if (ctl == ctl_button)
	    return 1;
    }
}

/*
 * main().
 */

/* Select an interface to initialize depending on compiler settings
 * and the user environment.
 */
static void initui(void)
{
#if defined INCLUDE_SDL
#if defined unix
    if (!getenv("DISPLAY"))
	goto skipsdl;
#endif
    if (initializeio(io_sdl))
	return;
    skipsdl:
#endif
#if defined INCLUDE_CURSES
    if (getenv("TERM"))
	if (initializeio(io_curses))
	    return;
#endif
    initializeio(io_text);
}

/* Run the program.
 */
int main(int argc, char *argv[])
{
    int i;

    (void)argv;
    if (argc > 1) {
	puts("Usage: yahtzee\n"
	     "This program takes no command-line arguments.\n"
	     "While the game is running, press ? or F1 for assistance.\n");
	for (i = 0 ; versioninfo[i] ; ++i)
	    puts(versioninfo[i]);
	return 0;
    }

    srand(time(0));
    initcontrols();
    initscoring();
    initui();

    while (playgame() && newgame()) ;
    return 0;
}
