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

#define PROBLEMS_MAX_YIELDER 16
#define PROBLEMS_MAX_SPINNER 8192

/*
 * Shared initialization routines
 */

static uint32_t startcount;
static struct lock *startlock;
static struct cv *startcv;
static struct semaphore *endsem;

static
void
inititems(uint32_t count)
{
	startcount = count;

	if (startlock==NULL) {
		startlock = lock_create("startlock");
		if (startlock == NULL) {
			panic("synchprobs: lock_create failed\n");
		}
	}
	if (startcv==NULL) {
		startcv = cv_create("startcv");
		if (startcv == NULL) {
			panic("synchprobs: cv_create failed\n");
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
male_wrapper(void * unused1, unsigned long index) {
	(void)unused1;

	random_yielder(4);
	lock_acquire(startlock);
	startcount--;
	if (startcount == 0) {
		cv_broadcast(startcv, startlock);
	} else {
		cv_wait(startcv, startlock);
	}
	lock_release(startlock);
	male((uint32_t)index);
	V(endsem);

	return;
}
void
male_start(uint32_t index) {
	(void)index;
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s starting\n", curthread->t_name);
}
void
male_end(uint32_t index) {
	(void)index;
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s ending\n", curthread->t_name);
}

static
void
female_wrapper(void * unused1, unsigned long index) {
	(void)unused1;

	random_yielder(4);
	lock_acquire(startlock);
	startcount--;
	if (startcount == 0) {
		cv_broadcast(startcv, startlock);
	} else {
		cv_wait(startcv, startlock);
	}
	lock_release(startlock);
	female((uint32_t)index);
	V(endsem);

	return;
}
void
female_start(uint32_t index) {
	(void) index;
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s starting\n", curthread->t_name);
}
void
female_end(uint32_t index) {
	(void) index;
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s ending\n", curthread->t_name);
}

static
void
matchmaker_wrapper(void * unused1, unsigned long index) {
	(void)unused1;
	
	random_yielder(4);
	lock_acquire(startlock);
	startcount--;
	if (startcount == 0) {
		cv_broadcast(startcv, startlock);
	} else {
		cv_wait(startcv, startlock);
	}
	lock_release(startlock);
	matchmaker((uint32_t)index);
	V(endsem);
	
	return;
}
void
matchmaker_start(uint32_t index) {
	(void)index;
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s starting\n", curthread->t_name);
}
void
matchmaker_end(uint32_t index) {
	(void)index;
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s ending\n", curthread->t_name);
}

#define NMATING 10

int
whalemating(int nargs, char **args) {
	(void) nargs;
	(void) args;

	int i, j, err = 0;
	char name[32];
	
	inititems(3 * NMATING);
	whalemating_init();

	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {
			switch (i) {
				case 0:
					snprintf(name, sizeof(name), "Male Whale Thread %d", (i * 3) + j);
					err = thread_fork(name, NULL, male_wrapper, NULL, j);
					break;
				case 1:
					snprintf(name, sizeof(name), "Female Whale Thread %d", (i * 3) + j);
					err = thread_fork(name, NULL, female_wrapper, NULL, j);
					break;
				case 2:
					snprintf(name, sizeof(name), "Matchmaker Whale Thread %d", (i * 3) + j);
					err = thread_fork(name, NULL, matchmaker_wrapper, NULL, j);
					break;
			}
			if (err) {
				panic("whalemating: thread_fork failed: (%s)\n", strerror(err));
			}
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
turnright_wrapper(void *index, unsigned long direction)
{
	random_yielder(4);
	lock_acquire(startlock);
	startcount--;
	if (startcount == 0) {
		cv_broadcast(startcv, startlock);
	} else {
		cv_wait(startcv, startlock);
	}
	lock_release(startlock);
	turnright((uint32_t)direction, (uint32_t)index);
	V(endsem);

	return;
}
static
void
gostraight_wrapper(void *index, unsigned long direction)
{
	random_yielder(4);
	lock_acquire(startlock);
	startcount--;
	if (startcount == 0) {
		cv_broadcast(startcv, startlock);
	} else {
		cv_wait(startcv, startlock);
	}
	lock_release(startlock);
	gostraight((uint32_t)direction, (uint32_t)index);
	V(endsem);

	return;
}
static
void
turnleft_wrapper(void *index, unsigned long direction)
{
	random_yielder(4);
	lock_acquire(startlock);
	startcount--;
	if (startcount == 0) {
		cv_broadcast(startcv, startlock);
	} else {
		cv_wait(startcv, startlock);
	}
	lock_release(startlock);
	turnleft((uint32_t)direction, (uint32_t)index);
	V(endsem);

	return;
}

void
inQuadrant(int quadrant, uint32_t index) {
	(void)index;

	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s in quadrant %d\n", curthread->t_name, quadrant);
}

void
leaveIntersection(uint32_t index) {
	(void)index;

	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s left the intersection\n", curthread->t_name);
}

#define NCARS 64

struct semaphore * stoplightMenuSemaphore;

int stoplight(int nargs, char **args) {
	(void) nargs;
	(void) args;
	int i, direction, turn, err = 0;
	char name[32];

	inititems(NCARS);
	stoplight_init();

	for (i = 0; i < NCARS; i++) {

		direction = random() % 4;
		turn = random() % 3;

		snprintf(name, sizeof(name), "Car Thread %d", i);

		switch (turn) {
			case 0:
			err = thread_fork(name, NULL, gostraight_wrapper, (void *)i, direction);
			break;
			case 1:
			err = thread_fork(name, NULL, turnleft_wrapper, (void *)i, direction);
			break;
			case 2:
			err = thread_fork(name, NULL, turnright_wrapper, (void *)i, direction);
			break;
		}
		if (err) {
			panic("stoplight: thread_fork failed: (%s)\n", strerror(err));
		}
	}
	
	for (i = 0; i < NCARS; i++) {
		P(endsem);
	}

	stoplight_cleanup();

	return 0;
}
