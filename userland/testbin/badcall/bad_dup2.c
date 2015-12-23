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
 * Invalid calls to dup2
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <err.h>

#include "config.h"
#include "test.h"

static
void
dup2_fd2(int fd, const char *desc)
{
	int rv;

	report_begin("%s", desc);
	rv = dup2(STDIN_FILENO, fd);
	report_check(rv, errno, EBADF);

	if (rv != -1) {
		close(fd);	/* just in case */
	}
}

static
void
dup2_self(void)
{
	struct stat sb;
	int rv;
	int testfd;

	/* use fd that isn't in use */
	testfd = CLOSED_FD;

	report_begin("copying stdin to test with");

	rv = dup2(STDIN_FILENO, testfd);
	if (rv == -1) {
		report_result(rv, errno);
		report_aborted();
		return;
	}

	report_begin("dup2 to same fd");
	rv = dup2(testfd, testfd);
	if (rv == testfd) {
		report_passed();
	}
	else if (rv<0) {
		report_result(rv, errno);
		report_failure();
	}
	else {
		report_warnx("returned %d instead", rv);
		report_failure();
	}

	report_begin("fstat fd after dup2 to itself");
	rv = fstat(testfd, &sb);
	if (errno == ENOSYS) {
		report_saw_enosys();
	}
	report_result(rv, errno);
	if (rv==0) {
		report_passed();
	}
	else if (errno != ENOSYS) {
		report_failure();
	}
	else {
		report_skipped();
		/* no support for fstat; try lseek */
		report_begin("lseek fd after dup2 to itself");
		rv = lseek(testfd, 0, SEEK_CUR);
		report_result(rv, errno);
		if (rv==0 || (rv==-1 && errno==ESPIPE)) {
			report_passed();
		}
		else {
			report_failure();
		}
	}

	close(testfd);
}

void
test_dup2(void)
{
	/* This does the first fd. */
	test_dup2_fd();

	/* Any interesting cases added here should also go in common_fds.c */
	dup2_fd2(-1, "dup2 to -1");
	dup2_fd2(-5, "dup2 to -5");
	dup2_fd2(IMPOSSIBLE_FD, "dup2 to impossible fd");
#ifdef OPEN_MAX
	dup2_fd2(OPEN_MAX, "dup2 to OPEN_MAX");
#else
	warnx("Warning: OPEN_MAX not defined - test skipped");
#endif

	dup2_self();
}
