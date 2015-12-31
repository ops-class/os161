#ifndef _SYNCHPROBS_H_
#define _SYNCHPROBS_H_

/*
 * Synchronization problem primitives.
 */

/*
 * whalemating.c.
 */

void whalemating_init(void);
void whalemating_cleanup(void);
void male(void *, unsigned long);
void female(void *, unsigned long);
void matchmaker(void *, unsigned long);
 
/*
 * stoplight.c.
 */

void gostraight(void *, unsigned long);
void turnleft(void *, unsigned long);
void turnright(void *, unsigned long);
void stoplight_init(void);
void stoplight_cleanup(void);

#endif /* _SYNCHPROBS_H_ */
