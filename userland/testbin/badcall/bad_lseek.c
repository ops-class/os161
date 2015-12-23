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
 * lseek
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#include "config.h"
#include "test.h"

static
void
lseek_fd_device(void)
{
	int fd, rv;

	report_begin("lseek on device");

	fd = open("null:", O_RDONLY);
	if (fd<0) {
		report_warn("opening null: failed");
		report_aborted();
		return;
	}

	rv = lseek(fd, 309, SEEK_SET);
	report_check(rv, errno, ESPIPE);

	close(fd);
}

static
void
lseek_file_stdin(void)
{
	int fd, fd2, rv, status;
	const char slogan[] = "There ain't no such thing as a free lunch";
	size_t len = strlen(slogan);
	pid_t pid;

	report_begin("lseek stdin when open on file");

	/* fork so we don't affect our own stdin */
	pid = fork();
	if (pid<0) {
		report_warn("fork failed");
		report_aborted();
		return;
	}
	else if (pid!=0) {
		/* parent */
		rv = waitpid(pid, &status, 0);
		if (rv<0) {
			report_warn("waitpid failed");
			report_aborted();
		}
		if (WIFSIGNALED(status)) {
			report_warnx("subprocess exited with signal %d",
				     WTERMSIG(status));
			report_aborted();
		}
		else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
			report_warnx("subprocess exited with code %d",
				     WEXITSTATUS(status));
			report_aborted();
		}
		return;
	}

	/* child */

	fd = open_testfile(NULL);
	if (fd<0) {
		_exit(0);
	}

	/*
	 * Move file to stdin.
	 * Use stdin (rather than stdout or stderr) to maximize the
	 * chances of detecting any special-case handling of fds 0-2.
	 * (Writing to stdin is fine as long as it's open for write,
	 * and it will be.)
	 */
	fd2 = dup2(fd, STDIN_FILENO);
	if (fd2<0) {
		report_warn("dup2 to stdin failed");
		close(fd);
		remove(TESTFILE);
		_exit(1);
	}
	if (fd2 != STDIN_FILENO) {
		report_warn("dup2 returned wrong file handle");
		close(fd);
		remove(TESTFILE);
		_exit(1);
	}
	close(fd);

	rv = write(STDIN_FILENO, slogan, len);
	if (rv<0) {
		report_warn("write to %s (via stdin) failed", TESTFILE);
		remove(TESTFILE);
		_exit(1);
	}

	if ((unsigned)rv != len) {
		report_warnx("write to %s (via stdin) got short count",
			     TESTFILE);
		remove(TESTFILE);
		_exit(1);
	}

	/* blah */
	report_skipped();

	rv = lseek(STDIN_FILENO, 0, SEEK_SET);
	report_begin("try 1: SEEK_SET");
	report_check(rv, errno, 0);

	rv = lseek(STDIN_FILENO, 0, SEEK_END);
	report_begin("try 2: SEEK_END");
	report_check(rv, errno, 0);

	remove(TESTFILE);
	_exit(0);
}

static
void
lseek_loc_negative(void)
{
	int fd, rv;

	report_begin("lseek to negative offset");

	fd = open_testfile(NULL);
	if (fd<0) {
		report_aborted();
		return;
	}

	rv = lseek(fd, -309, SEEK_SET);
	report_check(rv, errno, EINVAL);

	close(fd);
	remove(TESTFILE);
}

static
void
lseek_whence_inval(void)
{
	int fd, rv;

	report_begin("lseek with invalid whence code");

	fd = open_testfile(NULL);
	if (fd<0) {
		report_aborted();
		return;
	}

	rv = lseek(fd, 0, 3594);
	report_check(rv, errno, EINVAL);

	close(fd);
	remove(TESTFILE);
}

static
void
lseek_loc_pasteof(void)
{
	const char *message = "blahblah";
	int fd;
	off_t pos;

	report_begin("seek past/to EOF");

	fd = open_testfile(message);
	if (fd<0) {
		report_aborted();
		return;
	}

	pos = lseek(fd, 5340, SEEK_SET);
	if (pos == -1) {
		report_warn("lseek past EOF failed");
		report_failure();
		goto out;
	}
	if (pos != 5340) {
		report_warnx("lseek to 5340 got offset %lld", (long long) pos);
		report_failure();
		goto out;
	}

	pos = lseek(fd, -50, SEEK_CUR);
	if (pos == -1) {
		report_warn("small seek beyond EOF failed");
		report_failure();
		goto out;
	}
	if (pos != 5290) {
		report_warnx("SEEK_CUR to 5290 got offset %lld",
			     (long long) pos);
		report_failure();
		goto out;
	}

	pos = lseek(fd, 0, SEEK_END);
	if (pos == -1) {
		report_warn("seek to EOF failed");
		report_failure();
		goto out;
	}

	if (pos != (off_t) strlen(message)) {
		report_warnx("seek to EOF got %lld (should be %zu)",
			     (long long) pos, strlen(message));
		report_failure();
		goto out;
	}

	report_passed();

    out:
	close(fd);
	remove(TESTFILE);
	return;
}

void
test_lseek(void)
{
	test_lseek_fd();

	lseek_fd_device();
	lseek_file_stdin();
	lseek_loc_negative();
	lseek_loc_pasteof();
	lseek_whence_inval();
}
