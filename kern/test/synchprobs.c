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
 * Driver code for the whalemating problem.
 */

inline void male_start(void) {
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s starting\n", curthread->t_name);
}

inline void male_end(void) {
	tkprintf("%s ending\n", curthread->t_name);
}

inline void female_start(void) {
	random_spinner(PROBLEMS_MAX_SPINNER);
	random_yielder(PROBLEMS_MAX_YIELDER);
	tkprintf("%s starting\n", curthread->t_name);
}

inline void female_end(void) {
	tkprintf("%s ending\n", curthread->t_name);
}

inline void matchmaker_start(void) {
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	tkprintf("%s starting\n", curthread->t_name);
}

inline void matchmaker_end(void) {
	tkprintf("%s ending\n", curthread->t_name);
}

#define NMATING 10

struct semaphore * whalematingMenuSemaphore;

int whalemating(int nargs, char **args) {
	(void) nargs;
	(void) args;

	int i, j, err = 0;
	char name[32];

	whalematingMenuSemaphore = sem_create("Whalemating Driver Semaphore", 0);
	if (whalematingMenuSemaphore == NULL ) {
		panic("whalemating: sem_create failed.\n");
	}

	whalemating_init();

	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {

			random_yielder(PROBLEMS_MAX_YIELDER);

			switch (i) {
				case 0:
					snprintf(name, sizeof(name), "Male Whale Thread %d", (i * 3) + j);
					err = thread_fork(name, NULL, male, whalematingMenuSemaphore, j);
					break;
				case 1:
					snprintf(name, sizeof(name), "Female Whale Thread %d", (i * 3) + j);
					err = thread_fork(name, NULL, female, whalematingMenuSemaphore, j);
					break;
				case 2:
					snprintf(name, sizeof(name), "Matchmaker Whale Thread %d", (i * 3) + j);
					err = thread_fork(name, NULL, matchmaker, whalematingMenuSemaphore, j);
					break;
			}
			if (err) {
				panic("whalemating: thread_fork failed: (%s)\n", strerror(err));
			}
		}
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {
			P(whalematingMenuSemaphore);
		}
	}

	sem_destroy(whalematingMenuSemaphore);
	whalemating_cleanup();

	return 0;
}

/*
 * Driver code for the stoplight problem.
 */

inline void inQuadrant(int quadrant) {
	random_spinner(PROBLEMS_MAX_SPINNER);
	random_yielder(PROBLEMS_MAX_YIELDER);
	tkprintf("%s in quadrant %d\n", curthread->t_name, quadrant);
}

inline void leaveIntersection() {
	tkprintf("%s left the intersection\n", curthread->t_name);
}

#define NCARS 32

struct semaphore * stoplightMenuSemaphore;

int stoplight(int nargs, char **args) {
	(void) nargs;
	(void) args;
	int i, direction, turn, err = 0;
	char name[32];

	stoplightMenuSemaphore = sem_create("Stoplight Driver Semaphore", 0);
	if (stoplightMenuSemaphore == NULL ) {
		panic("stoplight: sem_create failed.\n");
	}

	stoplight_init();

	for (i = 0; i < NCARS; i++) {

		direction = random() % 4;
		turn = random() % 3;

		snprintf(name, sizeof(name), "Car Thread %d", i);

		switch (turn) {

			random_yielder(PROBLEMS_MAX_YIELDER);

			case 0:
			err = thread_fork(name, NULL, gostraight, stoplightMenuSemaphore, direction);
			break;
			case 1:
			err = thread_fork(name, NULL, turnleft, stoplightMenuSemaphore, direction);
			break;
			case 2:
			err = thread_fork(name, NULL, turnright, stoplightMenuSemaphore, direction);
			break;
		}
		if (err) {
			panic("stoplight: thread_fork failed: (%s)\n", strerror(err));
		}
	}

	for (i = 0; i < NCARS; i++) {
		P(stoplightMenuSemaphore);
	}

	sem_destroy(stoplightMenuSemaphore);
	stoplight_cleanup();

	return 0;
}
