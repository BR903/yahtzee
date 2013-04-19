/* yahtzee.h: Basic game definitions.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#ifndef _yahtzee_h_
#define _yahtzee_h_

/* The complete list of I/O control object ID values. (Macros are used
 * instead of an enum so that they can be used in array declarations.)
 */
#define ctl_dice_count		5
#define ctl_slots_count		16
#define	ctl_dice		(0)
#define	ctl_die_0		(ctl_dice + 0)
#define	ctl_die_1		(ctl_dice + 1)
#define	ctl_die_2		(ctl_dice + 2)
#define	ctl_die_3		(ctl_dice + 3)
#define	ctl_die_4		(ctl_dice + 4)
#define	ctl_dice_end		(ctl_dice + ctl_dice_count)
#define	ctl_button		(ctl_dice_end)
#define ctl_slots		(ctl_button + 1)
#define	ctl_slot_ones		(ctl_slots + 0)
#define	ctl_slot_twos		(ctl_slots + 1)
#define	ctl_slot_threes		(ctl_slots + 2)
#define	ctl_slot_fours		(ctl_slots + 3)
#define	ctl_slot_fives		(ctl_slots + 4)
#define	ctl_slot_sixes		(ctl_slots + 5)
#define	ctl_slot_subtotal	(ctl_slots + 6)
#define	ctl_slot_bonus		(ctl_slots + 7)
#define	ctl_slot_threeofakind	(ctl_slots + 8)
#define	ctl_slot_fourofakind	(ctl_slots + 9)
#define	ctl_slot_fullhouse	(ctl_slots + 10)
#define	ctl_slot_smallstraight	(ctl_slots + 11)
#define	ctl_slot_largestraight	(ctl_slots + 12)
#define	ctl_slot_yahtzee	(ctl_slots + 13)
#define	ctl_slot_chance		(ctl_slots + 14)
#define	ctl_slot_total		(ctl_slots + 15)
#define	ctl_slots_end		(ctl_slots + ctl_slots_count) 
#define	ctl_count		(ctl_slots_end)

/* Possible values for the button control. Each value corresponds to a
 * different button label.
 */
#define bval_roll		0
#define	bval_score		1
#define	bval_newgame		2
#define	bval_count		3

/* Input event actions.
 */
enum { act_null, act_over, act_out, act_down, act_up, act_clicked };

/* The information stored for each I/O control.
 */
struct control {
    int value: 16;		/* the value currently stored in the control */
    int key: 10;		/* the control's hot key */
    int disabled: 2;		/* true if the control is disabled for input */
    int set: 2;			/* true if the control has been activated */
    int hovering: 2;		/* true if the mouse is hovering over */
};

/* The array of I/O controls.
 */
extern struct control controls[ctl_count];

/* A pointer to the currently selected slot, or null if no slot is
 * selected.
 */
extern struct control *selected;

#endif
