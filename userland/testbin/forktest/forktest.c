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
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <test161/test161.h>

#define FORKTEST_FILENAME_BASE "forktest"

static char filename[32];

/*
 * This is used by all processes, to try to help make sure all
 * processes have a distinct address space.
 */
static volatile int mypid;

/*
 * Helper function to do pow()
 */
static
int pow_int(int x, int y) {
	int i;
	int result = 1;
	for(i = 0; i < y; i++)
		result *= x;
	return result;
}

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
	nprintf(".");
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
			errx(1, "waitpid");
		}
		else if (WIFSIGNALED(x)) {
			errx(1, "pid %d: signal %d", pid, WTERMSIG(x));
		}
		else if (WEXITSTATUS(x) != 0) {
			errx(1, "pid %d: exit %d", pid, WEXITSTATUS(x));
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

	/*
	 * Guru: We have a problem here.
	 * We need to write the output to a file since test161 is
	 * supposed to be as simple as possible. This requires the
	 * test to tell test161 whether it passed or succeeded.
	 * We cannot lseek and read stdout, to check the output,
	 * so we need to write the output to a file and then check it later.
	 *
	 * So far so good. However, if in the future, forktest is
	 * going to be combined with triple/quint/etc, then the filename
	 * cannot be the same across multiple copies. So we need a
	 * unique filename per instance of forktest.
	 * So...
	 */
	snprintf(filename, 32, "%s-%d.bin", FORKTEST_FILENAME_BASE, getpid());
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
	if(fd < 3) {
		// 0, 1, 2 are stdin, stdout, stderr
		err(1, "Failed to open file to write data into\n");
	}

	pid0 = dofork();
	nprintf(".");
	write(fd, "A", 1);
	depth++;
	if (depth != 1) {
		warnx("depth %d, should be 1", depth);
	}
	check();

	pid1 = dofork();
	nprintf(".");
	write(fd, "B", 1);
	depth++;
	if (depth != 2) {
		warnx("depth %d, should be 2", depth);
	}
	check();

	pid2 = dofork();
	nprintf(".");
	write(fd, "C", 1);
	depth++;
	if (depth != 3) {
		warnx("depth %d, should be 3", depth);
	}
	check();

	pid3 = dofork();
	nprintf(".");
	write(fd, "D", 1);
	depth++;
	if (depth != 4) {
		warnx("depth %d, should be 4", depth);
	}
	check();

	/*
	 * These must be called in reverse order to avoid waiting
	 * improperly.
	 */
	dowait(nowait, pid3);
	nprintf(".");
	dowait(nowait, pid2);
	nprintf(".");
	dowait(nowait, pid1);
	nprintf(".");
	dowait(nowait, pid0);
	nprintf(".");

	// Check if file contents are correct
	// lseek may not be implemented..so close and reopen
	close(fd);
	fd = open(filename, O_RDONLY);
	if(fd < 3) {
		err(1, "Failed to open file for verification\n");
	}
	nprintf(".");

	char buffer[30];
	int len;
	int char_idx, i;
	int observed, expected;
	char character = 'A';

	memset(buffer, 0, 30);
	len = read(fd, buffer, 30);
	printf("\n%s\n", buffer);
	if(len != 30) {
		err(1, "Did not get expected number of characters\n");
	}
	nprintf(".");
	// Check if number of instances of each character is correct
	// 2As; 4Bs; 8Cs; 16Ds
	for(char_idx = 0; char_idx < 4; char_idx++) {
		nprintf(".");
		observed = 0;
		expected = pow_int(2, char_idx + 1);
		for(i = 0; i < 30; i++) {
			// In C, char can be directly converted to an ASCII index
			// So, A is 65, B is 66, ...
			if(buffer[i] == character + char_idx) {
				observed++;
			}
		}
		if(observed != expected) {
			// Failed
			err(1, "Failed! Expected %d%cs..observed: %d\n", expected, character + char_idx, observed);
		}
	}
	nprintf("\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/forktest");
	close(fd);
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
