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
 * rmtest.c
 *
 * 	Tests file system synchronization by deleting an open file and
 * 	then attempting to read it.
 *
 * This should run correctly when the file system assignment is complete.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#define TEST        "rmdata"
#define TESTDATA    "I wish I was a headlight. -- Jerry Garcia"
#define TESTLEN     (sizeof(TESTDATA)-1)

static
void
dorm(int fd)
{
	/*
	 * This used to spawn a copy of /bin/rm, but that's silly.
	 * However, we will do the remove() from a subprocess, so
	 * that various kinds of improper hacks to make this test
	 * run won't work.
	 *
	 * Close the file in the subprocess, for similar reasons.
	 */

	pid_t pid;
	int status;

	pid = fork();
	if (pid<0) {
		err(1, "fork");
	}
	if (pid==0) {
		/* child process */
		close(fd);
		if (remove(TEST)) {
			err(1, "%s: remove", TEST);
		}
		_exit(0);
	}
	/* parent process */
	if (waitpid(pid, &status, 0)<0) {
		err(1, "waitpid");
	}
	else if (WIFSIGNALED(status)) {
		warn("child process exited with signal %d", WTERMSIG(status));
	}
	else if (WEXITSTATUS(status) != 0) {
		warnx("child process exited with code %d",WEXITSTATUS(status));
	}
}

static
int
same(const char *a, const char *b, int len)
{
	while (len-- > 0) {
		if (*a++ != *b++) return 0;
	}
	return 1;
}

int
main(void)
{
	int file, len;
	char buf[TESTLEN];

	/* create test data file */
	file = open(TEST, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	write(file, TESTDATA, TESTLEN);
	close(file);

	/* make sure the data is there */
	file = open(TEST, O_RDONLY);
	len = read(file, buf, TESTLEN);
	if (len < 0) {
		warn("read: before deletion");
	}
	else if (len < (int)TESTLEN) {
		warnx("read: before deletion: short count %d", len);
	}
	if (!same(buf, TESTDATA, TESTLEN)) {
		errx(1, "Failed: data read back was not the same");
	}

	/* rewind the file */
	if (lseek(file, 0, SEEK_SET)) {
		err(1, "lseek");
	}

	/* now spawn our killer and wait for it to do its work */
	dorm(file);

	/* we should be still able to read the data */
	memset(buf, '\0', TESTLEN);
	len = read(file, buf, TESTLEN);
	if (len < 0) {
		warn("read: after deletion");
	}
	else if (len < (int)TESTLEN) {
		warnx("read: after deletion: short count %d", len);
	}

	if (!same(buf, TESTDATA, TESTLEN)) {
		errx(1, "Failed: data read after deletion was not the same");
	}

	/* ok, close the file and it should go away */
	close(file);

	/* try to open it again */
	file = open(TEST, O_RDONLY);
	if (file >= 0) {
		close(file);
		errx(1, "Failed: the file could still be opened");
	}

	if (errno!=ENOENT) {
		err(1, "Unexpected error reopening the file");
	}

	tprintf("Succeeded!\n");

	return 0;
}
