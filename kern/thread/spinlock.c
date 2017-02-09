/*
 * Copyright (c) 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Make sure to build out-of-line versions of inline functions */
#define SPINLOCK_INLINE   /* empty */
#define MEMBAR_INLINE     /* empty */

#include <types.h>
#include <lib.h>
#include <cpu.h>
#include <spl.h>
#include <spinlock.h>
#include <membar.h>
#include <current.h>	/* for curcpu */

/*
 * Spinlocks.
 */


/*
 * Initialize spinlock.
 */
void
spinlock_init(struct spinlock *splk)
{
	spinlock_data_set(&splk->splk_lock, 0);
	splk->splk_holder = NULL;
	HANGMAN_LOCKABLEINIT(&splk->splk_hangman, "spinlock");
}

/*
 * Clean up spinlock.
 */
void
spinlock_cleanup(struct spinlock *splk)
{
	KASSERT(splk->splk_holder == NULL);
	KASSERT(spinlock_data_get(&splk->splk_lock) == 0);
}

/*
 * Get the lock.
 *
 * First disable interrupts (otherwise, if we get a timer interrupt we
 * might come back to this lock and deadlock), then use a machine-level
 * atomic operation to wait for the lock to be free.
 */
void
spinlock_acquire(struct spinlock *splk)
{
	struct cpu *mycpu;

	splraise(IPL_NONE, IPL_HIGH);

	/* this must work before curcpu initialization */
	if (CURCPU_EXISTS()) {
		mycpu = curcpu->c_self;
		if (splk->splk_holder == mycpu) {
			panic("Deadlock on spinlock %p\n", splk);
		}
		mycpu->c_spinlocks++;

		HANGMAN_WAIT(&curcpu->c_hangman, &splk->splk_hangman);
	}
	else {
		mycpu = NULL;
	}

	while (1) {
		/*
		 * Do test-test-and-set, that is, read first before
		 * doing test-and-set, to reduce bus contention.
		 *
		 * Test-and-set is a machine-level atomic operation
		 * that writes 1 into the lock word and returns the
		 * previous value. If that value was 0, the lock was
		 * previously unheld and we now own it. If it was 1,
		 * we don't.
		 */
		if (spinlock_data_get(&splk->splk_lock) != 0) {
			continue;
		}
		if (spinlock_data_testandset(&splk->splk_lock) != 0) {
			continue;
		}
		break;
	}

	membar_store_any();
	splk->splk_holder = mycpu;

	if (CURCPU_EXISTS()) {
		HANGMAN_ACQUIRE(&curcpu->c_hangman, &splk->splk_hangman);
	}
}

/*
 * Release the lock.
 */
void
spinlock_release(struct spinlock *splk)
{
	/* this must work before curcpu initialization */
	if (CURCPU_EXISTS()) {
		KASSERT(splk->splk_holder == curcpu->c_self);
		KASSERT(curcpu->c_spinlocks > 0);
		curcpu->c_spinlocks--;
		HANGMAN_RELEASE(&curcpu->c_hangman, &splk->splk_hangman);
	}

	splk->splk_holder = NULL;
	membar_any_store();
	spinlock_data_set(&splk->splk_lock, 0);
	spllower(IPL_HIGH, IPL_NONE);
}

/*
 * Check if the current cpu holds the lock.
 */
bool
spinlock_do_i_hold(struct spinlock *splk)
{
	if (!CURCPU_EXISTS()) {
		return true;
	}

	/* Assume we can read splk_holder atomically enough for this to work */
	return (splk->splk_holder == curcpu->c_self);
}
