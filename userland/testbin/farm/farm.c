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
 * farm.c
 *
 * 	Run a bunch of cpu pigs and one cat.
 *      The file it cats is "catfile". This should be created in advance.
 *
 * This test should itself run correctly when the basic system calls
 * are complete. It may be helpful for scheduler performance analysis.
 */

#include <unistd.h>
#include <err.h>

static char *hargv[2] = { (char *)"hog", NULL };
static char *cargv[3] = { (char *)"cat", (char *)"catfile", NULL };

#define MAXPROCS  6
static int pids[MAXPROCS], npids;

static
void
spawnv(const char *prog, char **argv)
{
	int pid = fork();
	switch (pid) {
	    case -1:
		err(1, "fork");
	    case 0:
		/* child */
		execv(prog, argv);
		err(1, "%s", prog);
	    default:
		/* parent */
		pids[npids++] = pid;
		break;
	}
}

static
void
waitall(void)
{
	int i, status;
	for (i=0; i<npids; i++) {
		if (waitpid(pids[i], &status, 0)<0) {
			warn("waitpid for %d", pids[i]);
		}
		else if (WIFSIGNALED(status)) {
			warnx("pid %d: signal %d", pids[i], WTERMSIG(status));
		}
		else if (WEXITSTATUS(status) != 0) {
			warnx("pid %d: exit %d", pids[i], WEXITSTATUS(status));
		}
	}
}

static
void
hog(void)
{
	spawnv("/testbin/hog", hargv);
}

static
void
cat(void)
{
	spawnv("/bin/cat", cargv);
}

int
main(void)
{
	hog();
	hog();
	hog();
	cat();

	waitall();

	return 0;
}
