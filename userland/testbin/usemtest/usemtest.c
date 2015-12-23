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
 * Simple test for the user-level semaphores provided by semfs, aka
 * "sem:".
 *
 * This should mostly run once you've implemented open, read, write,
 * and fork, and run fully once you've also implemented waitpid.
 *
 * The last part of the test will generally hang, sometimes in fork,
 * unless your filetable/open-file locking is just so.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

#define ONCELOOPS   3
#define TWICELOOPS  2
#define THRICELOOPS 1
#define LOOPS (ONCELOOPS + 2*TWICELOOPS + 3*THRICELOOPS)
#define NUMJOBS 4

/*
 * Print to the console, one character at a time to encourage
 * interleaving if the semaphores aren't working.
 */
static
void
say(const char *str)
{
	size_t i;

	for (i=0; str[i]; i++) {
		putchar(str[i]);
	}
}

#if 0 /* not used */
static
void
sayf(const char *str, ...)
{
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	say(str);
}
#endif

/*
 * This should probably be in libtest.
 */
static
void
dowait(pid_t pid, unsigned num)
{
	pid_t r;
	int status;

	r = waitpid(pid, &status, 0);
	if (r < 0) {
		warn("waitpid");
		return;
	}
	if (WIFSIGNALED(status)) {
		warnx("pid %d (subprocess %u): Signal %d", (int)pid,
		      num, WTERMSIG(status));
	}
	else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		warnx("pid %d (subprocess %u): Exit %d", (int)pid,
		     num, WEXITSTATUS(status));
	}
}

////////////////////////////////////////////////////////////
// semaphore access

/*
 * Semaphore structure.
 */
struct usem {
	char name[32];
	int fd;
};

static
void
usem_init(struct usem *sem, const char *tag, unsigned num)
{
	snprintf(sem->name, sizeof(sem->name), "sem:usemtest.%s%u", tag, num);
	sem->fd = open(sem->name, O_RDWR|O_CREAT|O_TRUNC, 0664);
	if (sem->fd < 0) {
		err(1, "%s: create", sem->name);
	}
	close(sem->fd);
	sem->fd = -1;
}

static
void
usem_open(struct usem *sem)
{
	sem->fd = open(sem->name, O_RDWR);
	if (sem->fd < 0) {
		err(1, "%s: open", sem->name);
	}
}

static
void
usem_close(struct usem *sem)
{
	if (close(sem->fd) == -1) {
		warn("%s: close", sem->name);
	}
}

static
void
usem_cleanup(struct usem *sem)
{
	(void)remove(sem->name);
}

static
void
P(struct usem *sem)
{
	ssize_t r;
	char c;

	r = read(sem->fd, &c, 1);
	if (r < 0) {
		err(1, "%s: read", sem->name);
	}
	if (r == 0) {
		errx(1, "%s: read: unexpected EOF", sem->name);
	}
}

static
void
V(struct usem *sem)
{
	ssize_t r;
	char c;

	r = write(sem->fd, &c, 1);
	if (r < 0) {
		err(1, "%s: write", sem->name);
	}
	if (r == 0) {
		errx(1, "%s: write: short count", sem->name);
	}
}

////////////////////////////////////////////////////////////
// test components

static
void
child_plain(struct usem *gosem, struct usem *waitsem, unsigned num)
{
	static const char *const strings[NUMJOBS] = {
		"Nitwit!",
		"Blubber!",
		"Oddment!",
		"Tweak!",
	};

	const char *string;
	unsigned i;

	string = strings[num];
	for (i=0; i<LOOPS; i++) {
		P(gosem);
		say(string);
		V(waitsem);
	}
}

static
void
child_with_own_fd(struct usem *gosem, struct usem *waitsem, unsigned num)
{
	usem_open(gosem);
	usem_open(waitsem);
	child_plain(gosem, waitsem, num);
	usem_close(gosem);
	usem_close(waitsem);
}

static
void
baseparent(struct usem *gosems, struct usem *waitsems)
{
	unsigned i, j;

	for (i=0; i<NUMJOBS; i++) {
		usem_open(&gosems[i]);
		usem_open(&waitsems[i]);
	}

	say("Once...\n");
	for (j=0; j<ONCELOOPS; j++) {
		for (i=0; i<NUMJOBS; i++) {
			V(&gosems[i]);
			P(&waitsems[i]);
			putchar(' ');
		}
		putchar('\n');
	}

	say("Twice...\n");
	for (j=0; j<TWICELOOPS; j++) {
		for (i=0; i<NUMJOBS; i++) {
			V(&gosems[i]);
			P(&waitsems[i]);
			putchar(' ');
			V(&gosems[i]);
			P(&waitsems[i]);
			putchar(' ');
		}
		putchar('\n');
	}

	say("Three times...\n");
	for (j=0; j<THRICELOOPS; j++) {
		for (i=0; i<NUMJOBS; i++) {
			V(&gosems[i]);
			P(&waitsems[i]);
			putchar(' ');
			V(&gosems[i]);
			P(&waitsems[i]);
			putchar(' ');
			V(&gosems[i]);
			P(&waitsems[i]);
			putchar('\n');
		}
	}

	for (i=0; i<NUMJOBS; i++) {
		usem_close(&gosems[i]);
		usem_close(&waitsems[i]);
	}
}

static
void
basetest(void)
{
	unsigned i;
	struct usem gosems[NUMJOBS], waitsems[NUMJOBS];
	pid_t pids[NUMJOBS];

	for (i=0; i<NUMJOBS; i++) {
		usem_init(&gosems[i], "g", i);
		usem_init(&waitsems[i], "w", i);
	}

	for (i=0; i<NUMJOBS; i++) {
		pids[i] = fork();
		if (pids[i] < 0) {
			err(1, "fork");
		}
		if (pids[i] == 0) {
			child_with_own_fd(&gosems[i], &waitsems[i], i);
			_exit(0);
		}
	}
	baseparent(gosems, waitsems);

	for (i=0; i<NUMJOBS; i++) {
		dowait(pids[i], i);
	}

	for (i=0; i<NUMJOBS; i++) {
		usem_cleanup(&gosems[i]);
		usem_cleanup(&waitsems[i]);
	}
}

static
void
concparent(struct usem *gosems, struct usem *waitsems)
{
	unsigned i, j;

	/*
	 * Print this *before* forking as we frequently hang *in* fork.
	 *say("Shoot...\n");
	 */
	for (j=0; j<LOOPS; j++) {
		for (i=0; i<NUMJOBS; i++) {
			V(&gosems[i]);
			P(&waitsems[i]);
			putchar(' ');
		}
		putchar('\n');
	}
}

static
void
conctest(void)
{
	unsigned i;
	struct usem gosems[NUMJOBS], waitsems[NUMJOBS];
	pid_t pids[NUMJOBS];

	say("Shoot...\n");

	for (i=0; i<NUMJOBS; i++) {
		usem_init(&gosems[i], "g", i);
		usem_init(&waitsems[i], "w", i);
		usem_open(&gosems[i]);
		usem_open(&waitsems[i]);
	}

	for (i=0; i<NUMJOBS; i++) {
		pids[i] = fork();
		if (pids[i] < 0) {
			err(1, "fork");
		}
		if (pids[i] == 0) {
			child_plain(&gosems[i], &waitsems[i], i);
			_exit(0);
		}
	}
	concparent(gosems, waitsems);

	for (i=0; i<NUMJOBS; i++) {
		dowait(pids[i], i);
	}

	for (i=0; i<NUMJOBS; i++) {
		usem_close(&gosems[i]);
		usem_close(&waitsems[i]);
		usem_cleanup(&gosems[i]);
		usem_cleanup(&waitsems[i]);
	}
}

////////////////////////////////////////////////////////////
// concurrent use test

int
main(void)
{
	basetest();
	conctest();
	say("Passed.\n");
	return 0;
}
