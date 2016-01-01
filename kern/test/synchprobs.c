/*
 * NO NOT MODIFY THIS FILE
 *
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <current.h>
#include <synch.h>
#include <synchprobs.h>

#define PROBLEMS_MAX_YIELDER 16
#define PROBLEMS_MAX_SPINNER 8192

/*
 * Shared initialization routines
 */

static struct semaphore *startsem;
static struct semaphore *endsem;

static
void
inititems(void)
{
	if (startsem==NULL) {
		startsem = sem_create("startsem", 0);
		if (startsem == NULL) {
			panic("synchprobs: sem_create failed\n");
		}
	}
	if (endsem==NULL) {
		endsem = sem_create("endsem", 0);
		if (endsem == NULL) {
			panic("synchprobs: sem_create failed\n");
		}
	}
}

/*
 * Driver code for the whalemating problem.
 */

static
void
male_wrapper(void * unused1, unsigned long unused2) {
	(void)unused1;
	(void)unused2;

	random_yielder(4);
	P(startsem);	
	male();
	V(endsem);

	return;
}
void
male_start(void) {
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s starting\n", curthread->t_name);
}
void
male_end(void) {
	tkprintf("%s ending\n", curthread->t_name);
}

static
void
female_wrapper(void * unused1, unsigned long unused2) {
	(void)unused1;
	(void)unused2;

	random_yielder(4);
	P(startsem);	
	female();
	V(endsem);

	return;
}
void
female_start(void) {
	random_spinner(PROBLEMS_MAX_SPINNER);
	random_yielder(PROBLEMS_MAX_YIELDER);
	tkprintf("%s starting\n", curthread->t_name);
}
void
female_end(void) {
	tkprintf("%s ending\n", curthread->t_name);
}

static
void
matchmaker_wrapper(void * unused1, unsigned long unused2) {
	(void)unused1;
	(void)unused2;
	
	random_yielder(4);
	P(startsem);	
	matchmaker();
	V(endsem);
	
	return;
}
void
matchmaker_start(void) {
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s starting\n", curthread->t_name);
}
void
matchmaker_end(void) {
	tkprintf("%s ending\n", curthread->t_name);
}

#define NMATING 10

int
whalemating(int nargs, char **args) {
	(void) nargs;
	(void) args;

	int i, j, err = 0;
	char name[32];
	
	inititems();
	whalemating_init();

	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {
			switch (i) {
				case 0:
					snprintf(name, sizeof(name), "Male Whale Thread %d", (i * 3) + j);
					err = thread_fork(name, NULL, male_wrapper, NULL, 0);
					break;
				case 1:
					snprintf(name, sizeof(name), "Female Whale Thread %d", (i * 3) + j);
					err = thread_fork(name, NULL, female_wrapper, NULL, 0);
					break;
				case 2:
					snprintf(name, sizeof(name), "Matchmaker Whale Thread %d", (i * 3) + j);
					err = thread_fork(name, NULL, matchmaker_wrapper, NULL, 0);
					break;
			}
			if (err) {
				panic("whalemating: thread_fork failed: (%s)\n", strerror(err));
			}
		}
	}
	
	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {
			V(startsem);
		}
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {
			P(endsem);
		}
	}

	whalemating_cleanup();

	return 0;
}

/*
 * Driver code for the stoplight problem.
 */

static
void
turnright_wrapper(void *unused, unsigned long direction)
{
	(void)unused;
	
	random_yielder(4);
	P(startsem);
	turnright((uint32_t)direction);	
	V(endsem);

	return;
}
static
void
gostraight_wrapper(void *unused, unsigned long direction)
{
	(void)unused;
	
	random_yielder(4);
	P(startsem);
	gostraight((uint32_t)direction);	
	V(endsem);

	return;
}
static
void
turnleft_wrapper(void *unused, unsigned long direction)
{
	(void)unused;
	
	random_yielder(4);
	P(startsem);
	turnleft((uint32_t)direction);
	V(endsem);

	return;
}

void
inQuadrant(int quadrant) {
	random_spinner(PROBLEMS_MAX_SPINNER);
	random_yielder(PROBLEMS_MAX_YIELDER);
	tkprintf("%s in quadrant %d\n", curthread->t_name, quadrant);
}

void
leaveIntersection() {
	tkprintf("%s left the intersection\n", curthread->t_name);
}

#define NCARS 32

struct semaphore * stoplightMenuSemaphore;

int stoplight(int nargs, char **args) {
	(void) nargs;
	(void) args;
	int i, direction, turn, err = 0;
	char name[32];

	inititems();
	stoplight_init();

	for (i = 0; i < NCARS; i++) {

		direction = random() % 4;
		turn = random() % 3;

		snprintf(name, sizeof(name), "Car Thread %d", i);

		switch (turn) {
			case 0:
			err = thread_fork(name, NULL, gostraight_wrapper, NULL, direction);
			break;
			case 1:
			err = thread_fork(name, NULL, turnleft_wrapper, NULL, direction);
			break;
			case 2:
			err = thread_fork(name, NULL, turnright_wrapper, NULL, direction);
			break;
		}
		if (err) {
			panic("stoplight: thread_fork failed: (%s)\n", strerror(err));
		}
	}
	
	for (i = 0; i < NCARS; i++) {
		V(startsem);
	}

	for (i = 0; i < NCARS; i++) {
		P(endsem);
	}

	stoplight_cleanup();

	return 0;
}
