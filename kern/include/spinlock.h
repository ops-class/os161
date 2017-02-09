/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

/*
 * Spinlocks. While the guts are machine-dependent, the structure and the
 * basic functions are supposed to be the same across all machines.
 */

#include <cdefs.h>
#include <hangman.h>

/* Inlining support - for making sure an out-of-line copy gets built */
#ifndef SPINLOCK_INLINE
#define SPINLOCK_INLINE INLINE
#endif

/* Get the machine-dependent bits. */
#include <machine/spinlock.h>

/*
 * Basic spinlock.
 *
 * Note that spinlocks are held by CPUs, not by threads.
 *
 * This structure is made public so spinlocks do not have to be
 * malloc'd; however, code that uses spinlocks should not look inside
 * the structure directly but always use the spinlock API functions.
 */
struct spinlock {
	volatile spinlock_data_t splk_lock; /* Memory word where we spin. */
	struct cpu *splk_holder;	    /* CPU holding this lock. */
	HANGMAN_LOCKABLE(splk_hangman);     /* Deadlock detector hook. */
};

/*
 * Initializer for cases where a spinlock needs to be static or global.
 */
#ifdef OPT_HANGMAN
#define SPINLOCK_INITIALIZER	{ SPINLOCK_DATA_INITIALIZER, NULL, \
				  HANGMAN_LOCKABLE_INITIALIZER }
#else
#define SPINLOCK_INITIALIZER	{ SPINLOCK_DATA_INITIALIZER, NULL }
#endif

/*
 * Spinlock functions.
 *
 * init		Initialize the contents of a spinlock.
 * cleanup	Opposite of init. Lock must be unlocked.
 *
 * acquire	Get the lock, spinning as necessary. Also disables interrupts.
 * release	Release the lock. May re-enable interrupts.
 *
 * do_i_hold	Check if the current CPU holds the lock.
 */

void spinlock_init(struct spinlock *lk);
void spinlock_cleanup(struct spinlock *lk);

void spinlock_acquire(struct spinlock *lk);
void spinlock_release(struct spinlock *lk);

bool spinlock_do_i_hold(struct spinlock *lk);


#endif /* _SPINLOCK_H_ */
