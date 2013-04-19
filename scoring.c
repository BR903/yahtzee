/* scoring.c: Scoring functions.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#include "yahtzee.h"
#include "scoring.h"

/* The score-evaluation functions for each slot.
 */
static int (*slotevalfunctions[ctl_count])(int[6]);

/*
 * The individual scoring functions for each slot. Each function
 * accepts an array of 6 ints, each int indicating the number of dice
 * currently showing that value. The return value is the number of
 * points that the dice values would currently earn for that slot.
 */

static int ones(int values[6])   { return 1 * values[0]; }
static int twos(int values[6])   { return 2 * values[1]; }
static int threes(int values[6]) { return 3 * values[2]; }
static int fours(int values[6])  { return 4 * values[3]; }
static int fives(int values[6])  { return 5 * values[4]; }
static int sixes(int values[6])  { return 6 * values[5]; }

static int chance(int values[6])
{
    int total, i;

    total = 0;
    for (i = 0 ; i < 6 ; ++i)
	total += (i + 1) * values[i];
    return total;
}

static int threeofakind(int values[6])
{
    int i;

    for (i = 0 ; i < 6 ; ++i)
	if (values[i] >= 3)
	    return chance(values);
    return 0;
}

static int fourofakind(int values[6])
{
    int i;

    for (i = 0 ; i < 6 ; ++i)
	if (values[i] >= 4)
	    return chance(values);
    return 0;
}

static int yahtzee(int values[6])
{
    int i;

    for (i = 0 ; i < 6 ; ++i)
	if (values[i] == 5)
	    return 50;
    return 0;
}

static int fullhouse(int values[6])
{
    int count, i;

    count = 0;
    for (i = 0 ; i < 6 ; ++i) {
	switch (values[i]) {
	  case 0:	continue;
	  case 1:	return 0;
	  case 4:	return 0;
	  case 5:	return 25;
	  default:	count += values[i];
	}
    }
    return count == 5 ? 25 : 0;
}

static int smallstraight(int values[6])
{
    if (values[2] && values[3])
	if ((values[0] && values[1]) || (values[1] && values[4])
				     || (values[4] && values[5]))
	    return 30;
    return 0;
}

static int largestraight(int values[6])
{
    if (values[1] == 1 && values[2] == 1 && values[3] == 1 && values[4] == 1)
	return 40;
    return 0;
}

/*
 * Exported functions.
 */

/* Initialize the slotevalfunctions array.
 */
void initscoring(void)
{
    slotevalfunctions[ctl_slot_ones] = ones;
    slotevalfunctions[ctl_slot_twos] = twos;
    slotevalfunctions[ctl_slot_threes] = threes;
    slotevalfunctions[ctl_slot_fours] = fours;
    slotevalfunctions[ctl_slot_fives] = fives;
    slotevalfunctions[ctl_slot_sixes] = sixes;
    slotevalfunctions[ctl_slot_threeofakind] = threeofakind;
    slotevalfunctions[ctl_slot_fourofakind] = fourofakind;
    slotevalfunctions[ctl_slot_fullhouse] = fullhouse;
    slotevalfunctions[ctl_slot_smallstraight] = smallstraight;
    slotevalfunctions[ctl_slot_largestraight] = largestraight;
    slotevalfunctions[ctl_slot_yahtzee] = yahtzee;
    slotevalfunctions[ctl_slot_chance] = chance;
}

#if 0
/* Compute the score for the given slot using the current dice.
 */
int scorevalue(int slotid)
{
    int values[6] = { 0, 0, 0, 0, 0, 0 };
    int i;

    for (i = ctl_dice ; i < ctl_dice_end ; ++i)
	++values[controls[i].value];
    return slotevalfunctions[slotid](values);
}
#endif

/* Compute the score for each open slot using the current dice.
 */
void updateopenslots(void)
{
    int values[6] = { 0, 0, 0, 0, 0, 0 };
    int i;

    for (i = ctl_dice ; i < ctl_dice_end ; ++i)
	++values[controls[i].value];
    for (i = ctl_slots ; i < ctl_slots_end ; ++i)
	if (!isdisabled(controls[i]))
	    controls[i].value = slotevalfunctions[i](values);
}

/* Update the values for the output-only scoring slots (subtotal,
 * total, and bonus).
 */
void updatescores(void)
{
    int total, setcount, i;

    total = 0;
    setcount = 0;
    for (i = ctl_slot_ones ; i <= ctl_slot_sixes ; ++i) {
	if (isdisabled(controls[i]) || isselected(controls[i])) {
	    ++setcount;
	    total += controls[i].value;
	}
    }
    if (setcount) {
	controls[ctl_slot_subtotal].value = total;
	if (total >= 63) {
	    controls[ctl_slot_bonus].value = 35;
	    total += 35;
	} else {
	    controls[ctl_slot_bonus].value = setcount == 6 ? 0 : -1;
	}
    } else {
	controls[ctl_slot_subtotal].value = -1;
	controls[ctl_slot_bonus].value = -1;
    }
    for (i = ctl_slot_threeofakind ; i <= ctl_slot_chance ; ++i) {
	if (isdisabled(controls[i]) || isselected(controls[i])) {
	    ++setcount;
	    total += controls[i].value;
	}
    }
    controls[ctl_slot_total].value = setcount ? total : -1;
}
