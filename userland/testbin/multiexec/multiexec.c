/*
 * Copyright (c) 2014
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
 * multiexec - stuff N procs into exec at once
 * usage: multiexec [-j N] [prog [arg...]]
 *
 * This can be used both to see what happens when you have a lot of
 * execs at once (its original purpose) by running ordinary programs
 * like pwd (the default) and also just as a workload generator /
 * convenient way to start lots of copies of things at once.
 *
 * Note that this uses execv directly (not execvp) so it doesn't
 * search $PATH for the program you want to run, and therefore it
 * needs full paths. One could make it use execvp; it doesn't because
 * that would complicate its coordinated startup logic, and also get
 * in the way of using it to debug execv.
 *
 * Some things to try:
 *    multiexec /bin/true
 *    multiexec /bin/cat foo (for some file foo)
 *    multiexec /testbin/add 3 8
 * Some more aggressive things to try:
 *    multiexec /testbin/factorial 15
 *    multiexec /testbin/bigexec
 *    multiexec /testbin/sort (once you have a VM system)
 * Some mean things:
 *    multiexec /testbin/forktest
 *    multiexec /testbin/bloat (once you have sbrk)
 *    multiexec /bin/sh (this makes a huge mess unless you have job control)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

////////////////////////////////////////////////////////////
// semaphores

/*
 * We open the semaphore separately in each process to avoid
 * filehandle-level locking problems. If you can't be "reading" and
 * "writing" the semaphore concurrently because of the open file
 * object lock, then using the same file handle for P and V will
 * deadlock. Also, if this same lock is used to protect the reference
 * count on the open file logic, fork will block if another process is
 * using the same file handle for P, and then we're deadlocked too.
 *
 * Ideally the open file / filetable code wouldn't have this problem,
 * as it makes e.g. console output from background jobs behave
 * strangely, but it's a common issue in practice and it's better for
 * tests to be immune to it.
 */

struct usem {
	char name[32];
	int fd;
};

static
void
semcreate(const char *tag, struct usem *sem)
{
	int fd;

	snprintf(sem->name, sizeof(sem->name), "sem:multiexec.%s.%d",
		 tag, (int)getpid());

	fd = open(sem->name, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	if (fd < 0) {
		err(1, "%s: create", sem->name);
	}
	close(fd);
}

static
void
semopen(struct usem *sem)
{
	sem->fd = open(sem->name, O_RDWR, 0664);
	if (sem->fd < 0) {
		err(1, "%s: open", sem->name);
	}
}

static
void
semclose(struct usem *sem)
{
	close(sem->fd);
}

static
void
semdestroy(struct usem *sem)
{
	remove(sem->name);
}

static
void
semP(struct usem *sem, size_t num)
{
	char c[num];

	if (read(sem->fd, c, num) < 0) {
		err(1, "%s: read", sem->name);
	}
	(void)c;
}

static
void
semV(struct usem *sem, size_t num)
{
	char c[num];

	/* semfs does not use these values, but be conservative */
	memset(c, 0, num);

	if (write(sem->fd, c, num) < 0) {
		err(1, "%s: write", sem->name);
	}
}

////////////////////////////////////////////////////////////
// test

#define SUBARGC_MAX 64
static char *subargv[SUBARGC_MAX];
static int subargc = 0;

static
void
spawn(int njobs)
{
	struct usem s1, s2;
	pid_t pids[njobs];
	int failed, status;
	int i;

	semcreate("1", &s1);
	semcreate("2", &s2);

	tprintf("Forking %d child processes...\n", njobs);

	for (i=0; i<njobs; i++) {
		pids[i] = fork();
		if (pids[i] == -1) {
			/* continue with the procs we have; cannot kill them */
			warn("fork");
			warnx("*** Only started %u processes ***", i);
			njobs = i;
			break;
		}
		if (pids[i] == 0) {
			/* child */
			semopen(&s1);
			semopen(&s2);
			semV(&s1, 1);
			semP(&s2, 1);
			semclose(&s1);
			semclose(&s2);
			execv(subargv[0], subargv);
			warn("execv: %s", subargv[0]);
			_exit(1);
		}
	}

	semopen(&s1);
	semopen(&s2);
	tprintf("Waiting for fork...\n");
	semP(&s1, njobs);
	tprintf("Starting the execs...\n");
	semV(&s2, njobs);

	failed = 0;
	for (i=0; i<njobs; i++) {
		if (waitpid(pids[i], &status, 0) < 0) {
			warn("waitpid");
			failed++;
		}
		else if (WIFSIGNALED(status)) {
			warnx("pid %d (child %d): Signal %d",
			      (int)pids[i], i, WTERMSIG(status));
			failed++;
		}
		else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
			warnx("pid %d (child %d): Exit %d",
			      (int)pids[i], i, WEXITSTATUS(status));
			failed++;
		}
	}
	if (failed > 0) {
		warnx("%d children failed", failed);
	}
	else {
		tprintf("Succeeded\n");
	}

	semclose(&s1);
	semclose(&s2);
	semdestroy(&s1);
	semdestroy(&s2);
}

int
main(int argc, char *argv[])
{
	static char default_prog[] = "/bin/pwd";

	int njobs = 12;
	int i;

	for (i=1; i<argc; i++) {
		if (!strcmp(argv[i], "-j")) {
			i++;
			if (argv[i] == NULL) {
				errx(1, "Option -j requires an argument");
			}
			njobs = atoi(argv[i]);
		}
#if 0 /* XXX we apparently don't have strncmp? */
		else if (!strncmp(argv[i], "-j", 2)) {
			njobs = atoi(argv[i] + 2);
		}
#endif
		else {
			subargv[subargc++] = argv[i];
			if (subargc >= SUBARGC_MAX) {
				errx(1, "Too many arguments");
			}
		}
	}

	if (subargc == 0) {
		subargv[subargc++] = default_prog;
	}
	subargv[subargc] = NULL;

	spawn(njobs);

	return 0;
}
