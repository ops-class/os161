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
 * Test if redirecting stdin and stdout works. (Doesn't check stderr.)
 *
 * Normally it should without any extra effort, provided that dup2 has
 * been implemented properly, but experience has shown that sometimes
 * people get the idea that exec should reset the stdin, stdout, and
 * stderr file handles to point to the console, which breaks things.
 * Don't do that.
 *
 * (This test also depends on fork working properly.)
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#define PATH_CAT "/bin/cat"
#define INFILE "redirect.in"
#define OUTFILE "redirect.out"

static const char slogan[] = "CECIDI, ET NON SURGERE POSSUM!\n";

static
int
doopen(const char *path, int openflags)
{
	int fd;

	fd = open(path, openflags, 0664);
	if (fd < 0) {
		err(1, "%s", path);
	}
	return fd;
}

static
void
dodup2(int ofd, int nfd, const char *file)
{
	int r;

	r = dup2(ofd, nfd);
	if (r < 0) {
		err(1, "%s: dup2", file);
	}
	if (r != nfd) {
		errx(1, "%s: dup2: Expected %d, got %d", nfd, r);
	}
}

static
void
doclose(int fd, const char *file)
{
	if (close(fd)) {
		warnx("%s: close", file);
	}
}

static
void
mkfile(void)
{
	int fd;
	ssize_t r;

	fd = doopen(INFILE, O_WRONLY|O_CREAT|O_TRUNC);

	r = write(fd, slogan, strlen(slogan));
	if (r < 0) {
		err(1, "%s: write", INFILE);
	}
	if ((size_t)r != strlen(slogan)) {
		errx(1, "%s: write: Short count (got %zd, expected %zu)",
		     INFILE, r, strlen(slogan));
	}

	doclose(fd, INFILE);
}

static
void
chkfile(void)
{
	char buf[256];
	ssize_t r;
	int fd;

	fd = doopen(OUTFILE, O_RDONLY);

	r = read(fd, buf, sizeof(buf));
	if (r < 0) {
		err(1, "%s: read", OUTFILE);
	}
	if (r == 0) {
		errx(1, "%s: read: Unexpected EOF", OUTFILE);
	}
	if ((size_t)r != strlen(slogan)) {
		errx(1, "%s: read: Short count (got %zd, expected %zu)",
		     OUTFILE, r, strlen(slogan));
	}

	doclose(fd, OUTFILE);
}

static
void
cat(void)
{
	pid_t pid;
	int rfd, wfd, result, status;
	const char *args[2];

	rfd = doopen(INFILE, O_RDONLY);
	wfd = doopen(OUTFILE, O_WRONLY|O_CREAT|O_TRUNC);

	pid = fork();
	if (pid < 0) {
		err(1, "fork");
	}

	if (pid == 0) {
		/* child */
		dodup2(rfd, STDIN_FILENO, INFILE);
		dodup2(wfd, STDOUT_FILENO, OUTFILE);
		doclose(rfd, INFILE);
		doclose(wfd, OUTFILE);
		args[0] = "cat";
		args[1] = NULL;
		execv(PATH_CAT, (char **)args);
		warn("%s: execv", PATH_CAT);
		_exit(1);
	}

	/* parent */
	doclose(rfd, INFILE);
	doclose(wfd, OUTFILE);

	result = waitpid(pid, &status, 0);
	if (result == -1) {
		err(1, "waitpid");
	}
	if (WIFSIGNALED(status)) {
		errx(1, "pid %d: Signal %d", (int)pid, WTERMSIG(status));
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		errx(1, "pid %d: Exit %d", (int)pid, WEXITSTATUS(status));
	}
}

int
main(void)
{
	printf("Creating %s...\n", INFILE);
	mkfile();

	printf("Running cat < %s > %s\n", INFILE, OUTFILE);
	cat();

	printf("Checking %s...\n", OUTFILE);
	chkfile();

	printf("Passed.\n");
	(void)remove(INFILE);
	(void)remove(OUTFILE);
	return 0;
}
