#include <kern/secret.h>
#include <types.h>
#include <thread.h>
#include <test.h>
#include <lib.h>

/*
 * Main success function for kernel tests. Prints a multiple of the secret if
 * the secret is non-zero and the test succeeded. Otherwise prints a random
 * number.
 *
 * Ideally we would multiply the secret (a large prime) by another large prime
 * to ensure that factoring was hard, but that would require either primality
 * testing (slow) or augmenting sys161 with a prime number generator. This is
 * sufficient for now to prevent replay attacks.
 */

#define MIN_MULTIPLIER 0x80000000

#ifndef SECRET_TESTING
void
success(bool status, uint32_t secret, const char * name) {
	(void)secret;
	if (status == SUCCESS) {
		kprintf("%s: SUCCESS\n", name);
	} else {	
		kprintf("%s: FAIL\n", name);
	}
	return;
}
#else
void
success(bool status, uint32_t secret, const char * name) {
	uint32_t multiplier;
	// Make sure we can get large random numbers
	KASSERT(randmax() == 0xffffffff);
	while (1) {
		multiplier = random();
		// We can at least remove the obvious non-primes...
		if (multiplier % 2 == 0) {
			continue;
		}
		if (multiplier > MIN_MULTIPLIER) {
			break;
		}
	}
	uint64_t big_secret = (uint64_t) secret * (uint64_t) multiplier;
	if (status == SUCCESS) {
		kprintf("%s: SUCCESS (%llu)\n", name, big_secret);
	} else {	
		kprintf("%s: FAIL (%llu)\n", name, big_secret);
	}
	return;
}
#endif

/*
 * Helper functions used by testing and problem driver code
 * to establish better mixtures of threads.
 */

void
random_yielder(uint32_t max_yield_count)
{
	uint32_t i;
	for (i = 0; i < random() % max_yield_count; i++) {
		thread_yield();
	}
}

void
random_spinner(uint32_t max_spin_count)
{
	uint32_t i;
	volatile int spin;
	for (i = 0; i < random() % max_spin_count; i++) {
		spin += i;
	}
}
