/* yahtzee.c: Basic game definitions.
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

/* The array of I/O controls.
 */
struct control controls[ctl_count];

/* A pointer to the currently selected slot, or null if no slot is
 * selected.
 */
struct control *selected = 0;

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
	controls[i].disabled = 0;
	controls[i].set = 0;
	controls[i].hovering = 0;
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
	controls[i].disabled = 0;
	controls[i].set = 0;
	controls[i].value = (int)((rand() * 6.0) / (double)RAND_MAX);
    }
}

/* Reroll the dice that are currently set.
 */
static void rolldice(void)
{
    int i;

    for (i = ctl_dice ; i < ctl_dice_end ; ++i) {
	if (controls[i].set) {
	    controls[i].value = (int)((rand() * 6.0) / (double)RAND_MAX);
	    controls[i].set = 0;
	}
    }
}

/* Mark the dice as fixed.
 */
static void fixdice(void)
{
    int i;

    for (i = ctl_dice ; i < ctl_dice_end ; ++i)
	controls[i].disabled = 1;
}

/* Erase the scores in all of the slots.
 */
static void clearallslots(void)
{
    int i;

    selected = 0;
    for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
	controls[i].value = -1;
	controls[i].set = controls[i].key == 0;
    }
}

/* Reset the scores of the open slots.
 */
static void resetslots(void)
{
    int i;

    for (i = ctl_slots ; i < ctl_slots_end ; ++i)
	if (!controls[i].set)
	    controls[i].value = -1;
}

/* Simulate a brief button press.
 */
static void flashbutton(void)
{
    static struct timespec const wait = { 0, 100000000 };
    struct control saved;

    saved = controls[ctl_button];
    controls[ctl_button].hovering = 1;
    controls[ctl_button].set = 1;
    render();
    nanosleep(&wait, 0);
    controls[ctl_button] = saved;
    render();
}

/*
 * The main loop.
 */

/* Run a single session of the game. Returns true if the game ran to
 * completion, or false if the user quit. This function embodies the
 * game's state machine.
 */
static int playgame(void)
{
    struct control *control;
    int slotopencount, rollcount;
    int ctl, act, i;

    clearallslots();
    rollalldice();
    rollcount = 1;
    controls[ctl_button].value = bval_roll;
    controls[ctl_button].disabled = 1;

    for (;;) {
	slotopencount = 0;
	for (i = ctl_slots ; i < ctl_slots_end ; ++i)
	    if (!controls[i].set)
		++slotopencount;
	if (slotopencount == 0) {
	    updatescores();
	    break;
	}
	if (rollcount == 3 || selected) {
	    controls[ctl_button].value = bval_score;
	    controls[ctl_button].disabled = !selected;
	    if (rollcount == 3)
		fixdice();
	} else {
	    controls[ctl_button].value = bval_roll;
	    controls[ctl_button].disabled = 1;
	    for (i = ctl_dice ; i < ctl_dice_end ; ++i)
		if (controls[i].set)
		    controls[ctl_button].disabled = 0;
	}    

	render();

	getnextevent:
	if (rollcount == 3 && slotopencount == 1) {
	    if (selected) {
		ctl = ctl_button;
		act = act_clicked;
	    } else {
		for (i = ctl_slots ; i < ctl_slots_end ; ++i) {
		    if (!controls[i].set) {
			ctl = i;
			act = act_clicked;
			break;
		    }
		}
	    }
	} else {
	    if (!getinputevent(&ctl, &act))
		return 0;
	}
	control = &controls[ctl];
	if (control->disabled)
	    goto getnextevent;
	if (act == act_over) {
	    for (i = 0 ; i < ctl_count ; ++i)
		controls[i].hovering = 0;
	    control->hovering = 1;
	} else if (act == act_out) {
	    control->hovering = 0;
	}

	if (ctl >= ctl_dice && ctl < ctl_dice_end) {
	    if (rollcount == 3)
		goto getnextevent;
	    if (act == act_down || act == act_clicked) {
		control->set = !control->set;
		if (selected) {
		    selected = 0;
		    updatescores();
		}
	    }
	    continue;
	}

	if (ctl >= ctl_slots && ctl < ctl_slots_end) {
	    if (control->set)
		goto getnextevent;
	    if (act == act_down || act == act_clicked) {
		selected = selected == control ? 0 : control;
		for (i = ctl_dice ; i < ctl_dice_end ; ++i)
		    controls[i].set = 0;
	    }
	    if (control->value == -1)
		control->value = scorevalue(ctl);
	    updatescores();
	    continue;
	}

	if (ctl == ctl_button) {
	    if (act == act_down) {
		control->set = 1;
	    } else if (act == act_up) {
		control->set = 0;
		if (control->hovering)
		    act = act_clicked;
	    } else if (act == act_clicked) {
		flashbutton();
	    }
	    if (act != act_clicked)
		continue;

	    if (control->value == bval_score) {
		if (!selected) {
		    control->disabled = 1;
		    goto getnextevent;
		}
		selected->set = 1;
		selected = 0;
		if (slotopencount > 1) {
		    rollalldice();
		    rollcount = 1;
		    resetslots();
		}
	    } else {
		if (rollcount == 3) {
		    control->disabled = 1;
		    goto getnextevent;
		}
		selected = 0;
		rolldice();
		++rollcount;
		resetslots();
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
    int ctl, act;

    controls[ctl_button].value = bval_newgame;
    controls[ctl_button].disabled = 0;
    for (;;) {
	render();
	if (!getinputevent(&ctl, &act))
	    return 0;
	if (ctl != ctl_button)
	    continue;
	switch (act) {
	  case act_over:
	    controls[ctl_button].hovering = 1;
	    break;
	  case act_out:
	    controls[ctl_button].hovering = 0;
	    break;
	  case act_down:
	    controls[ctl_button].set = 1;
	    break;
	  case act_up:
	    controls[ctl_button].set = 0;
	    if (controls[ctl_button].hovering)
		return 1;
	    break;
	  case act_clicked:
	    flashbutton();
	    return 1;
	}
    }
}

/*
 * main().
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
#if !defined INCLUDE_SDL
    initializeio(io_curses);
#elif !defined INCLUDE_CURSES
    initializeio(io_sdl);
#else
    initializeio(getenv("DISPLAY") ? io_sdl :
		 getenv("TERM") ? io_curses : io_text);
#endif
    while (playgame() && newgame()) ;
    return 0;
}
