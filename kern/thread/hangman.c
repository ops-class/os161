/*
 * Copyright (c) 2015
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

/*
 * Simple deadlock detector.
 */

#include <types.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <hangman.h>

static struct spinlock hangman_lock = SPINLOCK_INITIALIZER;

/*
 * Look for a path through the waits-for graph that goes from START to
 * TARGET.
 *
 * Because lockables can only be held by one actor, and actors can
 * only be waiting for one thing at a time, this turns out to be
 * quite simple.
 */
static
void
hangman_check(const struct hangman_lockable *start,
	      const struct hangman_actor *target)
{
	const struct hangman_actor *cur;

	cur = start->l_holding;
	while (cur != NULL) {
		if (cur == target) {
			goto found;
		}
		if (cur->a_waiting == NULL) {
			break;
		}
		cur = cur->a_waiting->l_holding;
	}
	return;

 found:
	/*
	 * None of this can change while we print it (that's the point
	 * of it being a deadlock) so drop hangman_lock while
	 * printing; otherwise we can come back via kprintf_spinlock
	 * and that makes a mess. But force splhigh() explicitly so
	 * the console prints in polled mode and to discourage other
	 * things from running in the middle of the printout.
	 */
	splhigh();
	spinlock_release(&hangman_lock);

	kprintf("hangman: Detected lock cycle!\n");
	kprintf("hangman: in %s (%p);\n", target->a_name, target);
	kprintf("hangman: waiting for %s (%p), but:\n", start->l_name, start);
	kprintf("   lockable %s (%p)\n", start->l_name, start);
	cur = start->l_holding;
	while (cur != target) {
		kprintf("   held by actor %s (%p)\n", cur->a_name, cur);
		kprintf("   waiting for lockable %s (%p)\n",
			cur->a_waiting->l_name, cur->a_waiting);
		cur = cur->a_waiting->l_holding;
	}
	kprintf("   held by actor %s (%p)\n", cur->a_name, cur);
	panic("Deadlock.\n");
}

/*
 * Note that a is about to wait for l.
 *
 * Note that there's no point calling this if a isn't going to wait,
 * because in that case l->l_holding will be null and the check
 * won't go anywhere.
 *
 * One could also maintain in memory a graph of all requests ever
 * seen, in order to detect lock order inversions that haven't
 * actually deadlocked; but there are a lot of ways in which this is
 * tricky and problematic. For now we'll settle for just detecting and
 * reporting deadlocks that do happen.
 */
void
hangman_wait(struct hangman_actor *a,
	     struct hangman_lockable *l)
{
	if (l == &hangman_lock.splk_hangman) {
		/* don't recurse */
		return;
	}

	spinlock_acquire(&hangman_lock);

	if (a->a_waiting != NULL) {
		spinlock_release(&hangman_lock);
		panic("hangman_wait: already waiting for something?\n");
	}

	hangman_check(l, a);
	a->a_waiting = l;

	spinlock_release(&hangman_lock);
}

void
hangman_acquire(struct hangman_actor *a,
		struct hangman_lockable *l)
{
	if (l == &hangman_lock.splk_hangman) {
		/* don't recurse */
		return;
	}

	spinlock_acquire(&hangman_lock);

	if (a->a_waiting != l) {
		spinlock_release(&hangman_lock);
		panic("hangman_acquire: not waiting for lock %s (%p)\n",
		      l->l_name, l);
	}
	if (l->l_holding != NULL) {
		spinlock_release(&hangman_lock);
		panic("hangman_acquire: lock %s (%p) still held by %s (%p)\n",
		      l->l_name, l, a->a_name, a);
	}

	l->l_holding = a;
	a->a_waiting = NULL;

	spinlock_release(&hangman_lock);
}

void
hangman_release(struct hangman_actor *a,
		struct hangman_lockable *l)
{
	if (l == &hangman_lock.splk_hangman) {
		/* don't recurse */
		return;
	}

	spinlock_acquire(&hangman_lock);

	if (a->a_waiting != NULL) {
		spinlock_release(&hangman_lock);
		panic("hangman_release: waiting for something?\n");
	}
	if (l->l_holding != a) {
		spinlock_release(&hangman_lock);
		panic("hangman_release: not the holder\n");
	}

	l->l_holding = NULL;

	spinlock_release(&hangman_lock);
}
