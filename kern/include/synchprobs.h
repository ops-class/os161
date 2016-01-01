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
void male(void);
void female(void);
void matchmaker(void);
 
/*
 * stoplight.c.
 */

void gostraight(uint32_t);
void turnleft(uint32_t);
void turnright(uint32_t);
void stoplight_init(void);
void stoplight_cleanup(void);

#endif /* _SYNCHPROBS_H_ */
