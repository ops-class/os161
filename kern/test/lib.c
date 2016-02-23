#include <types.h>
#include <thread.h>
#include <test.h>
#include <lib.h>

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
