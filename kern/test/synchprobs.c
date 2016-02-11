/*
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <current.h>
#include <synch.h>
#include <kern/secret.h>
#include <spinlock.h>

#define PROBLEMS_MAX_YIELDER 16
#define PROBLEMS_MAX_SPINNER 8192

#define SUCCESS 0
#define FAIL 1

/*
 * Shared initialization routines
 */

static uint32_t startcount;
static struct lock *testlock;
static struct cv *startcv;
static struct cv *endcv;
static struct semaphore *endsem;

struct spinlock status_lock;
static bool test_status = FAIL;

static
void
inititems(uint32_t count)
{
	startcount = count;

	if (testlock==NULL) {
		testlock = lock_create("testlock");
		if (testlock == NULL) {
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
	spinlock_init(&status_lock);
}

/*
 * Helper function to initialize the thread pool.
 */
static
void
initialize_thread(volatile void* threads[], uint32_t index) {
	if (threads[index] != NULL) {
		test_status = FAIL;
	}
	threads[index] = curthread->t_stack;
}

/*
 * Helper function to check whether current thread is valid.
 */
static
void
check_thread(volatile void* threads[], uint32_t index) {
	if (threads[index] != curthread->t_stack) {
		test_status = FAIL;
	}
}

/*
 * Driver code for the whalemating problem.
 */

#define NMATING 10
#define MALE 0
#define FEMALE 1
#define MATCHMAKER 2
#define CHECK_TIMES 32

static volatile int male_start_count;
static volatile int male_end_count;
static volatile int female_start_count;
static volatile int female_end_count;
static volatile int matchmaker_start_count;
static volatile int matchmaker_end_count;
static volatile int match_count;

static volatile int match_status[3 * NMATING];
static volatile void* whale_threads[3 * NMATING];
static volatile int whale_roles[3 * NMATING];
static struct semaphore *matcher_sem;

/*
 * Enforce male_start() and male_end() called from male thread.
 * Similar for female and matchmaker threads
 */
static
void
check_role(uint32_t index, int role) {
	if (whale_roles[index] != role) {
		test_status = FAIL;
	}
}

static
void
male_wrapper(void * unused1, unsigned long index) {
	(void)unused1;

	random_yielder(4);
	lock_acquire(testlock);
	initialize_thread(whale_threads, (uint32_t)index);
	whale_roles[index] = MALE;
	startcount--;
	lock_release(testlock);
	male((uint32_t)index);
	V(endsem);

	return;
}
void
male_start(uint32_t index) {
	(void)index;
	lock_acquire(testlock);
	check_thread(whale_threads, index);
	check_role(index, MALE);
	male_start_count++;
	cv_signal(startcv, testlock);
	lock_release(testlock);
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	kprintf_n("%s starting\n", curthread->t_name);
	kprintf_t(".");
}
void
male_end(uint32_t index) {
	(void)index;
	lock_acquire(testlock);
	check_thread(whale_threads, index);
	check_role(index, MALE);
	cv_signal(endcv, testlock);
	male_end_count++;
	lock_release(testlock);
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	kprintf_n("%s ending\n", curthread->t_name);
	kprintf_t(".");
}

static
void
female_wrapper(void * unused1, unsigned long index) {
	(void)unused1;

	random_yielder(4);
	lock_acquire(testlock);
	initialize_thread(whale_threads, (uint32_t)index);
	whale_roles[index] = FEMALE;
	startcount--;
	lock_release(testlock);
	female((uint32_t)index);
	V(endsem);

	return;
}
void
female_start(uint32_t index) {
	(void) index;
	lock_acquire(testlock);
	check_thread(whale_threads, index);
	check_role(index, FEMALE);
	female_start_count++;
	cv_signal(startcv, testlock);
	lock_release(testlock);
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	kprintf_n("%s starting\n", curthread->t_name);
	kprintf_t(".");
}
void
female_end(uint32_t index) {
	(void) index;
	lock_acquire(testlock);
	check_thread(whale_threads, index);
	check_role(index, FEMALE);
	cv_signal(endcv, testlock);
	female_end_count++;
	lock_release(testlock);
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	kprintf_n("%s ending\n", curthread->t_name);
	kprintf_t(".");
}

static
void
matchmaker_wrapper(void * unused1, unsigned long index) {
	(void)unused1;

	random_yielder(4);
	lock_acquire(testlock);
	initialize_thread(whale_threads, (uint32_t)index);
	whale_roles[index] = MATCHMAKER;
	startcount--;
	lock_release(testlock);
	matchmaker((uint32_t)index);
	V(endsem);

	return;
}
void
matchmaker_start(uint32_t index) {
	(void)index;
	lock_acquire(testlock);
	check_thread(whale_threads, index);
	check_role(index, MATCHMAKER);
	matchmaker_start_count++;
	cv_signal(startcv, testlock);
	lock_release(testlock);
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	kprintf_n("%s starting\n", curthread->t_name);
	kprintf_t(".");
	P(matcher_sem);
}
void
matchmaker_end(uint32_t index) {
	(void)index;
	lock_acquire(testlock);
	check_thread(whale_threads, index);
	check_role(index, MATCHMAKER);

	int i = match_count * 3;
	match_status[i] = male_start_count - male_end_count;
	match_status[i+1] = female_start_count - female_end_count;
	match_status[i+2] = matchmaker_start_count - matchmaker_end_count;

	match_count++;
	matchmaker_end_count++;
	cv_signal(endcv, testlock);
	lock_release(testlock);
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	kprintf_n("%s ending\n", curthread->t_name);
	kprintf_t(".");
}

static
void
check_zero(int count, const char *name) {
	(void)name;
	if (count != 0) {
		test_status = FAIL;
	}
}

int
whalemating(int nargs, char **args) {
	(void) nargs;
	(void) args;

	int i, j, err = 0;
	char name[32];

	male_start_count = 0 ;
	male_end_count = 0 ;
	female_start_count = 0 ;
	female_end_count = 0 ;
	matchmaker_start_count = 0;
	matchmaker_end_count = 0;
	match_count = 0;
	
	startcount = 3 * NMATING;
	testlock = lock_create("testlock");
	if (testlock == NULL) {
		panic("sp1: lock_create failed\n");
	}
	startcv = cv_create("startcv");
	if (startcv == NULL) {
		panic("sp1: cv_create failed\n");
	}
	endcv = cv_create("endcv");
	if (endcv == NULL) {
		panic("sp1: cv_create failed\n");
	}
	endsem = sem_create("endsem", 0);
	if (endsem == NULL) {
		panic("sp1: sem_create failed\n");
	}
	matcher_sem = sem_create("matcher_sem", 0);
	if (matcher_sem == NULL) {
		panic("sp1: sem_create failed\n");
	}
	spinlock_init(&status_lock);
	test_status = FAIL;

	whalemating_init();
	
	/* Start males and females only. */
	for (i = 0; i < 2; i++) {
		for (j = 0; j < NMATING; j++) {
			kprintf_t(".");
			int index = (i * NMATING) + j;
			whale_threads[index] = NULL;
			switch (i) {
				case 0:
					snprintf(name, sizeof(name), "Male Whale Thread %d", index);
					err = thread_fork(name, NULL, male_wrapper, NULL, index);
					break;
				case 1:
					snprintf(name, sizeof(name), "Female Whale Thread %d", index);
					err = thread_fork(name, NULL, female_wrapper, NULL, index);
					break;
			}
			if (err) {
				panic("whalemating: thread_fork failed: (%s)\n", strerror(err));
			}
		}
	}
	
	/* Wait for males and females to start. */	
	for (i = 0; i < NMATING * 2; i++) {
		kprintf_t(".");
		lock_acquire(testlock);
		cv_wait(startcv, testlock);
		lock_release(testlock);
	}
	
	/* Make sure nothing is happening... */
	bool loop_status = SUCCESS;
	for (i = 0; i < CHECK_TIMES && loop_status == SUCCESS; ) {
		kprintf_t(".");
		random_spinner(PROBLEMS_MAX_SPINNER);
		lock_acquire(testlock);
		if ((male_start_count != NMATING) || (female_start_count != NMATING) ||
				(matchmaker_start_count + male_end_count + female_end_count + matchmaker_end_count != 0)) {
			loop_status = FAIL;
		}
		lock_release(testlock);
	}
	if (loop_status == FAIL) {
		test_status = FAIL;
		goto done;
	}
	
	/* Create the matchmakers */
	for (j = 0; j < NMATING; j++) {
		kprintf_t(".");
		int index = (2 * NMATING) + j;
		whale_threads[index] = NULL;
		snprintf(name, sizeof(name), "Matchmaker Whale Thread %d", index);
		err = thread_fork(name, NULL, matchmaker_wrapper, NULL, index);
		if (err) {
			panic("whalemating: thread_fork failed: (%s)\n", strerror(err));
		}
	}
	
	/*
	 * Release a random number of matchmakers and wait for them and their
	 * matches to finish.
	 */ 
	int pivot = (random() % (NMATING - 2)) + 1;
	for (i = 0; i < pivot; i++) {
		kprintf_t(".");
		V(matcher_sem);
	}
	for (i = 0; i < 3 * pivot; i++) {
		kprintf_t(".");
		lock_acquire(testlock);
		cv_wait(endcv, testlock);
		lock_release(testlock);
	}
	
	/* Make sure nothing else is happening... */	
	bool loop_status = SUCCESS;
	for (i = 0; i < CHECK_TIMES && loop_status == SUCCESS; ) {
		kprintf_t(".");
		random_spinner(PROBLEMS_MAX_SPINNER);
		lock_acquire(testlock);
		if ((male_start_count != NMATING) || (female_start_count != NMATING) ||
				(male_end_count != pivot) || (female_end_count != pivot) ||
				(matchmaker_start_count != pivot) || (matchmaker_end_count != pivot)) {
			loop_status = FAIL;
		}
		lock_release(testlock);
	}
	if (loop_status == FAIL) {
		test_status = FAIL;
		goto done;
	}
	
	/*
	 * Release the rest of the matchmakers and wait for everyone to finish.
	 */

	for (i = pivot; i < NMATING; i++) {
		V(matcher_sem);
	}
	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {
			kprintf_t(".");
			P(endsem);
		}
	}

	whalemating_cleanup();

	check_zero(male_start_count - NMATING, "male");
	check_zero(female_start_count - NMATING, "female");
	check_zero(matchmaker_start_count - NMATING, "matchmaker");
	check_zero(male_start_count - male_end_count, "male");
	check_zero(female_start_count - female_end_count, "female");
	check_zero(matchmaker_start_count - matchmaker_end_count, "matchmaker");

	if (match_count == NMATING) {
		for (i = 0; i < NMATING; i++) {
			kprintf_t(".");
			j = i * 3;
			int male = match_status[j];
			int female = match_status[j + 1];
			if (male == 0 || female == 0) {
				spinlock_acquire(&status_lock);
				test_status = FAIL;
				spinlock_release(&status_lock);
			}
		}
	} else {
		spinlock_acquire(&status_lock);
		test_status = FAIL;
		spinlock_release(&status_lock);
	}

done:
	lock_destroy(testlock);
	cv_destroy(startcv);
	cv_destroy(endcv);
	sem_destroy(endsem);
	sem_destroy(matcher_sem);
	
	success(test_status, SECRET, "sp1");

	return 0;
}

/*
 * Driver code for the stoplight problem.
 */

#define NCARS 64
#define NUM_QUADRANTS 4
#define UNKNOWN_CAR -1
#define PASSED_CAR -2

#define GO_STRAIGHT 0
#define TURN_LEFT 1
#define TURN_RIGHT 2

static volatile int quadrant_array[NUM_QUADRANTS];
static volatile int max_car_count;
static volatile int all_quadrant;
static volatile int car_locations[NCARS];
static volatile int car_directions[NCARS];
static volatile int car_turns[NCARS];
static volatile int car_turn_times[NCARS];
static volatile void* car_threads[NCARS];

static
void
initialize_car_thread(uint32_t index, uint32_t direction, uint32_t turn) {
	initialize_thread(car_threads, index);
	car_directions[index] = direction;
	car_turns[index] = turn;
	car_turn_times[index] = 0;
}

static
void
check_intersection() {
	int n = 0;
	for (int i = 0; i < NUM_QUADRANTS; i++) {
		if (quadrant_array[i] > 1) {
			// panic("stoplight: more than 1 car in same quadrant %d\n", i);
			test_status = FAIL;
		}
		n += quadrant_array[i];
	}
	max_car_count = n > max_car_count ? n : max_car_count;
}

/*
 * When car move, must call this function and hold a lock.
 * It first checks current intersection status make sure no more than one car in one quadrant.
 * Then it removes current car from previous location.
 * In the end, it returns current car's index for inQuadrant, to let inQuadrant update car_locations array.
 */
static
int
move(uint32_t index) {
	check_thread(car_threads, index);
	check_intersection();
	int pre_location = car_locations[index];
	if (pre_location != UNKNOWN_CAR && pre_location != PASSED_CAR) {
		quadrant_array[pre_location]--;
	}
	return pre_location;
}

static
void
turnright_wrapper(void *index, unsigned long direction)
{
	random_yielder(4);
	lock_acquire(testlock);
	initialize_car_thread((uint32_t)index, (uint32_t)direction, TURN_RIGHT);
	startcount--;
	if (startcount == 0) {
		cv_broadcast(startcv, testlock);
	} else {
		cv_wait(startcv, testlock);
	}
	lock_release(testlock);
	turnright((uint32_t)direction, (uint32_t)index);
	V(endsem);

	return;
}
static
void
gostraight_wrapper(void *index, unsigned long direction)
{
	random_yielder(4);
	lock_acquire(testlock);
	initialize_car_thread((uint32_t)index, (uint32_t)direction, GO_STRAIGHT);
	startcount--;
	if (startcount == 0) {
		cv_broadcast(startcv, testlock);
	} else {
		cv_wait(startcv, testlock);
	}
	lock_release(testlock);
	gostraight((uint32_t)direction, (uint32_t)index);
	V(endsem);

	return;
}
static
void
turnleft_wrapper(void *index, unsigned long direction)
{
	random_yielder(4);
	lock_acquire(testlock);
	initialize_car_thread((uint32_t)index, (uint32_t)direction, TURN_LEFT);
	startcount--;
	if (startcount == 0) {
		cv_broadcast(startcv, testlock);
	} else {
		cv_wait(startcv, testlock);
	}
	lock_release(testlock);
	turnleft((uint32_t)direction, (uint32_t)index);
	V(endsem);

	return;
}

void
inQuadrant(int quadrant, uint32_t index) {
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	lock_acquire(testlock);
	int pre_quadrant = move(index);

	int target_quadrant = car_directions[index];
	switch (car_turn_times[index]) {
		case 0:
		if (pre_quadrant != UNKNOWN_CAR) {
			test_status = FAIL;
		}
		break;
		case 1:
		if (pre_quadrant != target_quadrant) {
			test_status = FAIL;
		}
		target_quadrant = (target_quadrant + NUM_QUADRANTS - 1) % NUM_QUADRANTS;
		break;
		case 2:
		target_quadrant = (target_quadrant + NUM_QUADRANTS - 1) % NUM_QUADRANTS;
		if (pre_quadrant != target_quadrant) {
			test_status = FAIL;
		}
		target_quadrant = (target_quadrant + NUM_QUADRANTS - 1) % NUM_QUADRANTS;
		break;
		default:
		test_status = FAIL;
		break;
	}
	if (quadrant != target_quadrant) {
		test_status = FAIL;
	}
	car_turn_times[index]++;

	if (quadrant_array[quadrant] > 0) {
		// panic("%s inQuadrant %d which already has another car\n", curthread->t_name, quadrant);
		test_status = FAIL;
	}
	quadrant_array[quadrant]++;
	car_locations[index] = quadrant;
	all_quadrant++;

	lock_release(testlock);
	kprintf_n("%s in quadrant %d\n", curthread->t_name, quadrant);
}

void
leaveIntersection(uint32_t index) {
	random_yielder(PROBLEMS_MAX_YIELDER);
	random_spinner(PROBLEMS_MAX_SPINNER);
	lock_acquire(testlock);
	move(index);

	switch (car_turns[index]) {
		case GO_STRAIGHT:
		if (car_turn_times[index] != 2) {
			test_status = FAIL;
		}
		break;
		case TURN_LEFT:
		if (car_turn_times[index] != 3) {
			test_status = FAIL;
		}
		break;
		case TURN_RIGHT:
		if (car_turn_times[index] != 1) {
			test_status = FAIL;
		}
		break;
		default:
		test_status = FAIL;
		break;
	}

	car_locations[index] = PASSED_CAR;
	lock_release(testlock);
	kprintf_n("%s left the intersection\n", curthread->t_name);
}

// TODO: should we delete this?
struct semaphore * stoplightMenuSemaphore;

int stoplight(int nargs, char **args) {
	(void) nargs;
	(void) args;
	int i, direction, turn, err = 0;
	char name[32];

	inititems(NCARS);
	stoplight_init();
	test_status = SUCCESS;

	max_car_count = 0;
	all_quadrant = 0;
	int required_quadrant = 0;

	for (i = 0; i < NUM_QUADRANTS; i++) {
		quadrant_array[i] = 0;
	}

	for (i = 0; i < NCARS; i++) {
		car_locations[i] = UNKNOWN_CAR;
		car_threads[i] = NULL;
		car_directions[i] = -1;
	}

	for (i = 0; i < NCARS; i++) {

		direction = random() % 4;
		turn = random() % 3;

		snprintf(name, sizeof(name), "Car Thread %d", i);

		switch (turn) {
			case GO_STRAIGHT:
			err = thread_fork(name, NULL, gostraight_wrapper, (void *)i, direction);
			required_quadrant += 2;
			break;
			case TURN_LEFT:
			err = thread_fork(name, NULL, turnleft_wrapper, (void *)i, direction);
			required_quadrant += 3;
			break;
			case TURN_RIGHT:
			err = thread_fork(name, NULL, turnright_wrapper, (void *)i, direction);
			required_quadrant += 1;
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

	if (test_status == FAIL) {
		success(test_status, SECRET, "sp2");
		return 0;
	}

	// Check all cars pass the intersection
	int passed = 0;
	for (i = 0; i < NCARS; i++) {
		passed += car_locations[i] == PASSED_CAR ? 1 : 0;
	}
	if (passed != NCARS) {
		// panic("stoplight: not all cars pass the intersection, total: %d, passed: %d\n", NCARS, passed);
		success(FAIL, SECRET, "sp2");
		return 0;
	}

	// Check hit quadrant times is same as required, for example, student can force all cars turn right
	if (all_quadrant != required_quadrant) {
		// panic("stoplight: you may make wrong turn for some cars\n");
		success(FAIL, SECRET, "sp2");
		return 0;
	}

	if (max_car_count <= 1) {
		test_status = FAIL;
		// panic("stoplight: only one car in the intersection at same time, did you use big lock?\n");
	}

	success(test_status, SECRET, "sp2");

	return 0;
}
