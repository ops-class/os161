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

/*
 * More thread test code.
 */
#include <types.h>
#include <lib.h>
#include <wchan.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

/* dimension of matrices (cannot be too large or will overflow stack) */

#define DIM 70

/* number of iterations for sleepalot threads */
#define SLEEPALOT_PRINTS      20	/* number of printouts */
#define SLEEPALOT_ITERS       4		/* iterations per printout */
/* polling frequency of waker thread */
#define WAKER_WAKES          100
/* number of iterations per compute thread */
#define COMPUTE_ITERS         10

/* N distinct wait channels */
#define NWAITCHANS 12
static struct spinlock spinlocks[NWAITCHANS];
static struct wchan *waitchans[NWAITCHANS];

static volatile int wakerdone;
static struct semaphore *wakersem;
static struct semaphore *donesem;

static
void
setup(void)
{
	char tmp[16];
	int i;

	if (wakersem == NULL) {
		wakersem = sem_create("wakersem", 1);
		donesem = sem_create("donesem", 0);
		for (i=0; i<NWAITCHANS; i++) {
			spinlock_init(&spinlocks[i]);
			snprintf(tmp, sizeof(tmp), "wc%d", i);
			waitchans[i] = wchan_create(kstrdup(tmp));
		}
	}
	wakerdone = 0;
}

static
void
sleepalot_thread(void *junk, unsigned long num)
{
	int i, j;

	(void)junk;

	for (i=0; i<SLEEPALOT_PRINTS; i++) {
		for (j=0; j<SLEEPALOT_ITERS; j++) {
			unsigned n;
			struct spinlock *lk;
			struct wchan *wc;

			n = random() % NWAITCHANS;
			lk = &spinlocks[n];
			wc = waitchans[n];
			spinlock_acquire(lk);
			wchan_sleep(wc, lk);
			spinlock_release(lk);
		}
		kprintf("[%lu]", num);
	}
	V(donesem);
}

static
void
waker_thread(void *junk1, unsigned long junk2)
{
	int i, done;

	(void)junk1;
	(void)junk2;

	while (1) {
		P(wakersem);
		done = wakerdone;
		V(wakersem);
		if (done) {
			break;
		}

		for (i=0; i<WAKER_WAKES; i++) {
			unsigned n;
			struct spinlock *lk;
			struct wchan *wc;

			n = random() % NWAITCHANS;
			lk = &spinlocks[n];
			wc = waitchans[n];
			spinlock_acquire(lk);
			wchan_wakeall(wc, lk);
			spinlock_release(lk);

			thread_yield();
		}
	}
	V(donesem);
}

static
void
make_sleepalots(int howmany)
{
	char name[16];
	int i, result;

	for (i=0; i<howmany; i++) {
		snprintf(name, sizeof(name), "sleepalot%d", i);
		result = thread_fork(name, NULL, sleepalot_thread, NULL, i);
		if (result) {
			panic("thread_fork failed: %s\n", strerror(result));
		}
	}
	result = thread_fork("waker", NULL, waker_thread, NULL, 0);
	if (result) {
		panic("thread_fork failed: %s\n", strerror(result));
	}
}

static
void
compute_thread(void *junk1, unsigned long num)
{
	struct matrix {
		char m[DIM][DIM];
	};
	struct matrix *m1, *m2, *m3;
	unsigned char tot;
	int i, j, k, m;
	uint32_t rand;

	(void)junk1;

	m1 = kmalloc(sizeof(struct matrix));
	KASSERT(m1 != NULL);
	m2 = kmalloc(sizeof(struct matrix));
	KASSERT(m2 != NULL);
	m3 = kmalloc(sizeof(struct matrix));
	KASSERT(m3 != NULL);

	for (m=0; m<COMPUTE_ITERS; m++) {

		for (i=0; i<DIM; i++) {
			for (j=0; j<DIM; j++) {
				rand = random();
				m1->m[i][j] = rand >> 16;
				m2->m[i][j] = rand & 0xffff;
			}
		}

		for (i=0; i<DIM; i++) {
			for (j=0; j<DIM; j++) {
				tot = 0;
				for (k=0; k<DIM; k++) {
					tot += m1->m[i][k] * m2->m[k][j];
				}
				m3->m[i][j] = tot;
			}
		}

		tot = 0;
		for (i=0; i<DIM; i++) {
			tot += m3->m[i][i];
		}

		kprintf("{%lu: %u}", num, (unsigned) tot);
		thread_yield();
	}

	kfree(m1);
	kfree(m2);
	kfree(m3);

	V(donesem);
}

static
void
make_computes(int howmany)
{
	char name[16];
	int i, result;

	for (i=0; i<howmany; i++) {
		snprintf(name, sizeof(name), "compute%d", i);
		result = thread_fork(name, NULL, compute_thread, NULL, i);
		if (result) {
			panic("thread_fork failed: %s\n", strerror(result));
		}
	}
}

static
void
finish(int howmanytotal)
{
	int i;
	for (i=0; i<howmanytotal; i++) {
		P(donesem);
	}
	P(wakersem);
	wakerdone = 1;
	V(wakersem);
	P(donesem);
}

static
void
runtest3(int nsleeps, int ncomputes)
{
	setup();
	kprintf("Starting thread test 3 (%d [sleepalots], %d {computes}, "
		"1 waker)\n",
		nsleeps, ncomputes);
	make_sleepalots(nsleeps);
	make_computes(ncomputes);
	finish(nsleeps+ncomputes);
	kprintf("\nThread test 3 done\n");
}

int
threadtest3(int nargs, char **args)
{
	if (nargs==1) {
		runtest3(5, 2);
	}
	else if (nargs==3) {
		runtest3(atoi(args[1]), atoi(args[2]));
	}
	else {
		kprintf("Usage: tt3 [sleepthreads computethreads]\n");
		return 1;
	}
	return 0;
}
