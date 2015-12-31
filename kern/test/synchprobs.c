/*
 * 08 Feb 2012 : GWA : Please make any changes necessary to test your code to
 * the drivers in this file. However, the automated testing suite *will
 * replace this file in its entirety* with driver code intented to stress
 * test your synchronization problem solutions.
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
 * 08 Feb 2012 : GWA : Driver code for the whalemating problem.
 */

/*
 * 08 Feb 2012 : GWA : The following functions are for you to use when each
 * whale starts and completes either mating (if it is a male or female) or
 * matchmaking. We will use the output from these functions to verify the to
 * verify the correctness of your solution. These functions may spin for
 * arbitrary periods of time or yield.
 */

inline void male_start(void) {
	random_yielder(PROBLEMS_MAX_YIELDER);
	kprintf("%s starting\n", curthread->t_name);
}

inline void male_end(void) {
	kprintf("%s ending\n", curthread->t_name);
}

inline void female_start(void) {
	random_spinner(PROBLEMS_MAX_SPINNER);
	kprintf("%s starting\n", curthread->t_name);
}

inline void female_end(void) {
	kprintf("%s ending\n", curthread->t_name);
}

inline void matchmaker_start(void) {
	random_yielder(PROBLEMS_MAX_YIELDER);
	kprintf("%s starting\n", curthread->t_name);
}

inline void matchmaker_end(void) {
	kprintf("%s ending\n", curthread->t_name);
}

/*
 * 08 Feb 2012 : GWA : The following function drives the entire whalemating
 * process. Feel free to modify at will, but make no assumptions about the
 * order or timing of threads launched by our testing suite.
 */

#define NMATING 10

struct semaphore * whalematingMenuSemaphore;

int whalemating(int nargs, char **args) {
	(void) nargs;
	(void) args;

	int i, j, err = 0;
	char name[32];

	whalematingMenuSemaphore = sem_create("Whalemating Driver Semaphore",
			0);
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
 * 08 Feb 2012 : GWA : Driver code for the stoplight problem.
 */

/*
 * 08 Feb 2012 : GWA : The following functions should be called by your
 * stoplight solution when a car is in an intersection quadrant. The
 * semantics of the problem are that once a car enters any quadrant it has to
 * be somewhere in the intersection until it call leaveIntersection(), which
 * it should call while in the final quadrant.
 *
 * As an example, let's say a car approaches the intersection and needs to
 * pass through quadrants 0, 3 and 2. Once you call inQuadrant(0), the car is
 * considered in quadrant 0 until you call inQuadrant(3). After you call
 * inQuadrant(2), the car is considered in quadrant 2 until you call
 * leaveIntersection().
 * 
 * As in the whalemating example, we will use the output from these functions
 * to verify the correctness of your solution. These functions may spin for
 * arbitrary periods of time or yield.
 */

inline void inQuadrant(int quadrant) {
	random_spinner(PROBLEMS_MAX_SPINNER);
	kprintf("%s in quadrant %d\n", curthread->t_name, quadrant);
}

inline void leaveIntersection() {
	kprintf("%s left the intersection\n", curthread->t_name);
}

#define NCARS 99

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
