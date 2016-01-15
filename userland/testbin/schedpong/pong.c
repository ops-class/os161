/*
 * Copyright (c) 2015
 *	The President and Fellows of Harvard College.
 *      Written by David A. Holland.
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
 * Semaphore pong.
 */

#include <stdio.h>
#include <err.h>
#include <assert.h>

#include "usem.h"
#include "tasks.h"

#define MAXCOUNT 64
#define PONGLOOPS 1000
//#define VERBOSE_PONG

static struct usem sems[MAXCOUNT];
static unsigned nsems;

/*
 * Set up the semaphores. This happens in the task director process,
 * so if we have multiple pong groups each has its own sems[] array.
 * (at least if the VM works)
 *
 * Note that we don't open the semaphores in the director process;
 * that way each task process has its own file handles and they don't
 * interfere with each other if file handle locking isn't so great.
 */
void
pong_prep(unsigned groupid, unsigned count)
{
	unsigned i;

	if (count > MAXCOUNT) {
		err(1, "pong: too many pongers -- recompile pong.c");
	}
	for (i=0; i<count; i++) {
		usem_init(&sems[i], "sem:pong-%u-%u", groupid, i);
	}
	nsems = count;
}

void
pong_cleanup(unsigned groupid, unsigned count)
{
	unsigned i;

	assert(nsems == count);
	(void)groupid;
	
	for (i=0; i<count; i++) {
		usem_cleanup(&sems[i]);
	}
}

/*
 * Pong in order. Wait on our semaphore, then wake the next one.
 * If we're id 0, don't wait the first go so things start, but do
 * wait the last go.
 */
static
void
pong_cyclic(unsigned id)
{
	unsigned i;
	unsigned nextid;

	nextid = (id + 1) % nsems;
	for (i=0; i<PONGLOOPS; i++) {
		if (i > 0 || id > 0) {
			P(&sems[id]);
		}
#ifdef VERBOSE_PONG
		tprintf(" %u", id);
#else
		if (nextid == 0 && i % 16 == 0) {
			putchar('.');
		}
#endif
		V(&sems[nextid]);
	}
	if (id == 0) {
		P(&sems[id]);
	}
#ifdef VERBOSE_PONG
	putchar('\n');
#else
	if (nextid == 0) {
		putchar('\n');
	}
#endif
}

/*
 * Pong back and forth. This runs the tasks with middle numbers more
 * often.
 */
static
void
pong_reciprocating(unsigned id)
{
	unsigned i, n;
	unsigned nextfwd, nextback;
	unsigned gofwd = 1;

	if (id == 0) {
		nextfwd = nextback = 1;
		n = PONGLOOPS;
	}
	else if (id == nsems - 1) {
		nextfwd = nextback = nsems - 2;
		n = PONGLOOPS;
	}
	else {
		nextfwd = id + 1;
		nextback = id - 1;
		n = PONGLOOPS * 2;
	}

	for (i=0; i<n; i++) {
		if (i > 0 || id > 0) {
			P(&sems[id]);
		}
#ifdef VERBOSE_PONG
		tprintf(" %u", id);
#else
		if (id == 0 && i % 16 == 0) {
			putchar('.');
		}
#endif
		if (gofwd) {
			V(&sems[nextfwd]);
			gofwd = 0;
		}
		else {
			V(&sems[nextback]);
			gofwd = 1;
		}
	}
	if (id == 0) {
		P(&sems[id]);
	}
#ifdef VERBOSE_PONG
	putchar('\n');
#else
	if (id == 0) {
		putchar('\n');
	}
#endif
}

/*
 * Do the pong thing.
 */
void
pong(unsigned groupid, unsigned id)
{
	unsigned idfwd, idback;

	(void)groupid;

	idfwd = (id + 1) % nsems;
	idback = (id + nsems - 1) % nsems;
	usem_open(&sems[id]);
	usem_open(&sems[idfwd]);
	usem_open(&sems[idback]);

	waitstart();
	pong_cyclic(id);
#ifdef VERBOSE_PONG
	tprintf("--------------------------------\n");
#endif
	pong_reciprocating(id);
#ifdef VERBOSE_PONG
	tprintf("--------------------------------\n");
#endif
	pong_cyclic(id);

	usem_close(&sems[id]);
	usem_close(&sems[idfwd]);
	usem_close(&sems[idback]);
}
