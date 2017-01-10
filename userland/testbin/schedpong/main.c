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

#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "usem.h"
#include "tasks.h"
#include "results.h"

#define STARTSEM "sem:start"

struct usem startsem;

/*
 * Task hook function that does nothing.
 */
static
void
nop(unsigned groupid, unsigned count)
{
	(void)groupid;
	(void)count;
}

/*
 * Wrapper for wait.
 */
static
unsigned
dowait(pid_t pid)
{
	int r;
	int status;

	r = waitpid(pid, &status, 0);
	if (r < 0) {
		err(1, "waitpid");
	}
	if (WIFSIGNALED(status)) {
		warnx("pid %d signal %d", pid, WTERMSIG(status));
		return 1;
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		warnx("pid %d exit %d", pid, WEXITSTATUS(status));
		return 1;
	}
	return 0;
}

/*
 * Do a task group: fork the processes, then wait for them.
 */
static
void
runtaskgroup(unsigned count,
	     void (*prep)(unsigned, unsigned),
	     void (*task)(unsigned, unsigned),
	     void (*cleanup)(unsigned, unsigned),
	     unsigned groupid)
{
	pid_t mypids[count];
	unsigned i;
	unsigned failures = 0;
	time_t secs;
	unsigned long nsecs;

	prep(groupid, count);

	for (i=0; i<count; i++) {
		mypids[i] = fork();
		if (mypids[i] < 0) {
			err(1, "fork");
		}
		if (mypids[i] == 0) {
			/* child (of second fork) */
			task(groupid, i);
			exit(0);
		}
		/* parent (of second fork) - continue */
	}

	/*
	 * now wait for the task to finish
	 */

	for (i=0; i<count; i++) {
		failures += dowait(mypids[i]);
	}

	/*
	 * Store the end time.
	 */

	__time(&secs, &nsecs);
	openresultsfile(O_WRONLY);
	putresult(groupid, secs, nsecs);
	closeresultsfile();

	cleanup(groupid, count);

	exit(failures ? 1 : 0);
}

/*
 * Fork the task groups. We will two tiers of fork: fork once to get a
 * process to own the task group, and then within the task group again
 * N times to get the processes to do the task. This way we can wait
 * for the different collections of task processes independently and
 * get timing results even on kernels that don't support waitpid with
 * WNOHANG.
 */
static
void
forkem(unsigned count,
       void (*prep)(unsigned, unsigned),
       void (*task)(unsigned, unsigned),
       void (*cleanup)(unsigned, unsigned),
       unsigned groupid,
       pid_t *retpid)
{
	*retpid = fork();
	if (*retpid < 0) {
		err(1, "fork");
	}
	if (*retpid == 0) {
		/* child */
		runtaskgroup(count, prep, task, cleanup, groupid);
	}
	/* parent -- just return */
}

/*
 * Wait for the task group directors to exit.
 */
static
void
waitall(pid_t *pids, unsigned numpids)
{
	unsigned failures = 0;
	unsigned i;

	for (i=0; i<numpids; i++) {
		failures += dowait(pids[i]);
	}
	if (failures) {
		errx(1, "TEST FAILURE: one or more subprocesses broke");
	}
}

/*
 * Fetch, compute, and print the timing for one task group.
 */
static
void
calcresult(unsigned groupid, time_t startsecs, unsigned long startnsecs,
	   char *buf, size_t bufmax)
{
	time_t secs;
	unsigned long nsecs;

	getresult(groupid, &secs, &nsecs);

	/* secs.nsecs -= startsecs.startnsecs */
	if (nsecs < startnsecs) {
		nsecs += 1000000000;
		secs--;
	}
	nsecs -= startnsecs;
	secs -= startsecs;
	snprintf(buf, bufmax, "%lld.%09lu", (long long)secs, nsecs);
}

/*
 * Used by the tasks to wait to start.
 */
void
waitstart(void)
{
	usem_open(&startsem);
	P(&startsem);
	usem_close(&startsem);
}

/*
 * Run the whole workload.
 */
static
void
runit(unsigned numthinkers, unsigned numgrinders,
      unsigned numponggroups, unsigned ponggroupsize)
{
	pid_t pids[numponggroups + 2];
	time_t startsecs;
	unsigned long startnsecs;
	char buf[32];
	unsigned i;

	tprintf("Running with %u thinkers, %u grinders, and %u pong groups "
	       "of size %u each.\n", numthinkers, numgrinders, numponggroups,
	       ponggroupsize);

	usem_init(&startsem, STARTSEM);
	createresultsfile();
	forkem(numthinkers, nop, think, nop, 0, &pids[0]);
	forkem(numgrinders, nop, grind, nop, 1, &pids[1]);
	for (i=0; i<numponggroups; i++) {
		forkem(ponggroupsize, pong_prep, pong, pong_cleanup, i+2,
		       &pids[i+2]);
	}
	usem_open(&startsem);
	tprintf("Forking done; starting the workload.\n");
	__time(&startsecs, &startnsecs);
	Vn(&startsem, numthinkers + numgrinders +
	   numponggroups * ponggroupsize);
	waitall(pids, numponggroups + 2);
	usem_close(&startsem);
	usem_cleanup(&startsem);

	openresultsfile(O_RDONLY);

	tprintf("--- Timings ---\n");
	if (numthinkers > 0) {
		calcresult(0, startsecs, startnsecs, buf, sizeof(buf));
		tprintf("Thinkers: %s\n", buf);
	}

	if (numgrinders > 0) {
		calcresult(1, startsecs, startnsecs, buf, sizeof(buf));
		tprintf("Grinders: %s\n", buf);
	}

	for (i=0; i<numponggroups; i++) {
		calcresult(i+2, startsecs, startnsecs, buf, sizeof(buf));
		tprintf("Pong group %u: %s\n", i, buf);
	}

	closeresultsfile();
	destroyresultsfile();
}

static
void
usage(const char *av0)
{
	warnx("Usage: %s [options]", av0);
	warnx("  [-t thinkers]         set number of thinkers (default 2)");
	warnx("  [-g grinders]         set number of grinders (default 0)");
	warnx("  [-p ponggroups]       set number of pong groups (default 1)");
	warnx("  [-s ponggroupsize]    set pong group size (default 6)");
	warnx("Thinkers are CPU bound; grinders are memory-bound;");
	warnx("pong groups are I/O bound.");
	exit(1);
}

int
main(int argc, char *argv[])
{
	unsigned numthinkers = 2;
	unsigned numgrinders = 0;
	unsigned numponggroups = 1;
	unsigned ponggroupsize = 6;

	int i;

	for (i=1; i<argc; i++) {
		if (!strcmp(argv[i], "-t")) {
			numthinkers = atoi(argv[++i]);
		}
		else if (!strcmp(argv[i], "-g")) {
			numgrinders = atoi(argv[++i]);
		}
		else if (!strcmp(argv[i], "-p")) {
			numponggroups = atoi(argv[++i]);
		}
		else if (!strcmp(argv[i], "-s")) {
			ponggroupsize = atoi(argv[++i]);
		}
		else {
			usage(argv[0]);
		}
	}

	runit(numthinkers, numgrinders, numponggroups, ponggroupsize);
	return 0;
}
