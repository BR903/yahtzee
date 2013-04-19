/* scoring.c: Scoring functions.
 *
 * Copyright (C) 2010 Brian Raiter.
 * This program is free software. See README for details.
 */

#ifndef _scoring_h_
#define _scoring_h_

/* Set up the scoring functions.
 */
extern void initscoring(void);

/* Compute the score for the given slot using the current dice.
 */
extern int scorevalue(int slotid);

/* Update the values for the output-only scoring slots.
 */
extern void updatescores(void);

#endif
