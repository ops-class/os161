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
 * Automation test code for creating (and detecting) kernel dead and livelocks.
 */

#include <types.h>
#include <kern/wait.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/secret.h>
#include <spinlock.h>

#define MAX_SPINNERS 16

static struct lock *deadlock_locks[2];
static struct semaphore *deadlock_sem;

struct spinlock spinners_lock[MAX_SPINNERS];

static
void
inititems(void)
{
	int i;

	for (i = 0; i < 2; i++) {
		deadlock_locks[i] = lock_create("deadlock lock");
		if (deadlock_locks[i] == NULL) {
			panic("automationtest: lock_create failed\n");
		}
	}
	deadlock_sem = sem_create("deadlock sem", 0);
	if (deadlock_sem == NULL) {
		panic("automationtest: sem_create failed\n");
	}
	
	for (i = 0; i < MAX_SPINNERS; i++) {
		spinlock_init(&(spinners_lock[i]));
	}
}

static
void
dltestthread(void *junk1, unsigned long junk2)
{
	(void)junk1;
	(void)junk2;
	
	lock_acquire(deadlock_locks[1]);	
	V(deadlock_sem);
	lock_acquire(deadlock_locks[0]);	
}

int
dltest(int nargs, char **args)
{
	int result;

	(void)nargs;
	(void)args;
	
	inititems();
	
	lock_acquire(deadlock_locks[0]);	

	result = thread_fork("dltest", NULL, dltestthread, NULL, (unsigned long)0);
	if (result) {
		panic("dltest: thread_fork failed: %s\n", strerror(result));
	}

	P(deadlock_sem);
	lock_acquire(deadlock_locks[1]);	

	panic("dltest: didn't create deadlock (locks probably don't work)\n");
	
	// 09 Jan 2015 : GWA : Shouldn't return.
	return 0;
}

inline
static
void
infinite_spinner(unsigned long i)
{
	(void)i;
	volatile int j;

	for (j=0; j<10000000; j++);

	spinlock_acquire(&(spinners_lock[i]));
	
	for (j=0; j<=1000; j++) {
		if (j == 1000) {
			j = 0;
		}
	}
	panic("ll1test: infinite spin loop completed\n");
}

int
ll1test(int nargs, char **args)
{
	(void)nargs;
	(void)args;
	
	inititems();

	infinite_spinner((unsigned long) 0);

	// 09 Jan 2015 : GWA : Shouldn't return.
	return 0;
}


static
void
ll16testthread(void *junk1, unsigned long i)
{
	(void)junk1;

	infinite_spinner(i);
}

int
ll16test(int nargs, char **args)
{
	int i, result;
	
	inititems();

	(void)nargs;
	(void)args;

	for (i=1; i<16; i++) {
		result = thread_fork("ll16testthread", NULL, ll16testthread, NULL, (unsigned long)i);
		if (result) {
			panic("ll16test: thread_fork failed: %s\n", strerror(result));
		}
	}
	infinite_spinner(0);

	// 09 Jan 2015 : GWA : Shouldn't return.
	return 0;
}
