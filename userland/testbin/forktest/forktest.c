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
 * forktest - test fork().
 *
 * This should work correctly when fork is implemented.
 *
 * It should also continue to work after subsequent assignments, most
 * notably after implementing the virtual memory system.
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

/*
 * This is used by all processes, to try to help make sure all
 * processes have a distinct address space.
 */
static volatile int mypid;

/*
 * Helper function for fork that prints a warning on error.
 */
static
int
dofork(void)
{
	int pid;
	pid = fork();
	if (pid < 0) {
		warn("fork");
	}
	return pid;
}

/*
 * Check to make sure each process has its own address space. Write
 * the pid into the data segment and read it back repeatedly, making
 * sure it's correct every time.
 */
static
void
check(void)
{
	int i;

	mypid = getpid();

	/* Make sure each fork has its own address space. */
	for (i=0; i<800; i++) {
		volatile int seenpid;
		seenpid = mypid;
		if (seenpid != getpid()) {
			errx(1, "pid mismatch (%d, should be %d) "
			     "- your vm is broken!",
			     seenpid, getpid());
		}
	}
}

/*
 * Wait for a child process.
 *
 * This assumes dowait is called the same number of times as dofork
 * and passed its results in reverse order. Any forks that fail send
 * us -1 and are ignored. The first 0 we see indicates the fork that
 * generated the current process; that means it's time to exit. Only
 * the parent of all the processes returns from the chain of dowaits.
 */
static
void
dowait(int nowait, int pid)
{
	int x;

	if (pid<0) {
		/* fork in question failed; just return */
		return;
	}
	if (pid==0) {
		/* in the fork in question we were the child; exit */
		exit(0);
	}

	if (!nowait) {
		if (waitpid(pid, &x, 0)<0) {
			warn("waitpid");
		}
		else if (WIFSIGNALED(x)) {
			warnx("pid %d: signal %d", pid, WTERMSIG(x));
		}
		else if (WEXITSTATUS(x) != 0) {
			warnx("pid %d: exit %d", pid, WEXITSTATUS(x));
		}
	}
}

/*
 * Actually run the test.
 */
static
void
test(int nowait)
{
	int pid0, pid1, pid2, pid3;
	int depth = 0;

	/*
	 * Caution: This generates processes geometrically.
	 *
	 * It is unrolled to encourage gcc to registerize the pids,
	 * to prevent wait/exit problems if fork corrupts memory.
	 *
	 * Note: if the depth prints trigger and show that the depth
	 * is too small, the most likely explanation is that the fork
	 * child is returning from the write() inside putchar()
	 * instead of from fork() and thus skipping the depth++. This
	 * is a fairly common problem caused by races in the kernel
	 * fork code.
	 */

	pid0 = dofork();
	depth++;
	putchar('A');
	if (depth != 1) {
		warnx("depth %d, should be 1", depth);
	}
	check();

	pid1 = dofork();
	depth++;
	putchar('B');
	if (depth != 2) {
		warnx("depth %d, should be 2", depth);
	}
	check();

	pid2 = dofork();
	depth++;
	putchar('C');
	if (depth != 3) {
		warnx("depth %d, should be 3", depth);
	}
	check();

	pid3 = dofork();
	depth++;
	putchar('D');
	if (depth != 4) {
		warnx("depth %d, should be 4", depth);
	}
	check();

	/*
	 * These must be called in reverse order to avoid waiting
	 * improperly.
	 */
	dowait(nowait, pid3);
	dowait(nowait, pid2);
	dowait(nowait, pid1);
	dowait(nowait, pid0);

	putchar('\n');
}

int
main(int argc, char *argv[])
{
	static const char expected[] =
		"|----------------------------|\n";
	int nowait=0;

	if (argc==2 && !strcmp(argv[1], "-w")) {
		nowait=1;
	}
	else if (argc!=1 && argc!=0) {
		warnx("usage: forktest [-w]");
		return 1;
	}
	warnx("Starting. Expect this many:");
	write(STDERR_FILENO, expected, strlen(expected));

	test(nowait);

	warnx("Complete.");
	return 0;
}
