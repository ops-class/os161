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
 * Calls with invalid fds
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


enum rwtestmodes {
	RW_TEST_NONE,
	RW_TEST_RDONLY,
	RW_TEST_WRONLY,
};

static
int
read_badfd(int fd)
{
	char buf[128];
	return read(fd, buf, sizeof(buf));
}

static
int
write_badfd(int fd)
{
	char buf[128];
	memset(buf, 'a', sizeof(buf));
	return write(fd, buf, sizeof(buf));
}


static
int
close_badfd(int fd)
{
	return close(fd);
}

static
int
ioctl_badfd(int fd)
{
	return ioctl(fd, 0, NULL);
}

static
int
lseek_badfd(int fd)
{
	return lseek(fd, 0, SEEK_SET);
}

static
int
fsync_badfd(int fd)
{
	return fsync(fd);
}

static
int
ftruncate_badfd(int fd)
{
	return ftruncate(fd, 60);
}

static
int
fstat_badfd(int fd)
{
	struct stat sb;
	return fstat(fd, &sb);
}

static
int
getdirentry_badfd(int fd)
{
	char buf[32];
	return getdirentry(fd, buf, sizeof(buf));
}

static
int
dup2_badfd(int fd)
{
	/* use the +1 to avoid doing dup2(CLOSED_FD, CLOSED_FD) */
	return dup2(fd, CLOSED_FD+1);
}

static
void
dup2_cleanup(void)
{
	close(CLOSED_FD+1);
}

////////////////////////////////////////////////////////////

static
void
any_badfd(int (*func)(int fd), void (*cleanup)(void), const char *callname,
	  int fd, const char *fddesc)
{
	int rv;

	report_begin("%s using %s", callname, fddesc);
	rv = func(fd);
	report_check(rv, errno, EBADF);
	if (cleanup) {
		cleanup();
	}
}

static
void
runtest(int (*func)(int fd), void (*cleanup)(void), const char *callname,
	enum rwtestmodes rw)
{
	int fd;

	/*
	 * If adding cases, also see bad_dup2.c
	 */

	/* basic invalid case: fd -1 */
	any_badfd(func, cleanup, callname, -1, "fd -1");

	/* also try -5 in case -1 is special somehow */
	any_badfd(func, cleanup, callname, -5, "fd -5");

	/* try a fd we know is closed */
	any_badfd(func, cleanup, callname, CLOSED_FD, "closed fd");

	/* try a positive fd we know is out of range */
	any_badfd(func, cleanup, callname, IMPOSSIBLE_FD, "impossible fd");

	/* test for off-by-one errors */
#ifdef OPEN_MAX
	any_badfd(func, cleanup, callname, OPEN_MAX, "fd OPEN_MAX");
#else
	warnx("Warning: OPEN_MAX not defined, test skipped");
#endif

	if (rw == RW_TEST_RDONLY) {
		fd = reopen_testfile(O_RDONLY|O_CREAT);
		if (fd < 0) {
			/* already printed a message */
		}
		else {
			any_badfd(func, cleanup, callname, fd,
				  "fd opened read-only");
		}
		close(fd);
	}
	if (rw == RW_TEST_WRONLY) {
		fd = reopen_testfile(O_WRONLY|O_CREAT);
		if (fd < 0) {
			/* already printed a message */
		}
		else {
			any_badfd(func, cleanup, callname, fd,
				  "fd opened write-only");
		}
		close(fd);
	}
}

////////////////////////////////////////////////////////////

#define T(call, rw) \
  void                                          \
  test_##call##_fd(void)                        \
  {                                             \
   	runtest(call##_badfd, NULL, #call, rw); \
  }

#define TC(call, rw) \
  void                                          \
  test_##call##_fd(void)                        \
  {                                             \
   	runtest(call##_badfd, call##_cleanup, #call, rw);\
  }

T(read, RW_TEST_WRONLY);
T(write, RW_TEST_RDONLY);
T(close, RW_TEST_NONE);
T(ioctl, RW_TEST_NONE);
T(lseek, RW_TEST_NONE);
T(fsync, RW_TEST_NONE);
T(ftruncate, RW_TEST_RDONLY);
T(fstat, RW_TEST_NONE);
T(getdirentry, RW_TEST_WRONLY);
TC(dup2, RW_TEST_NONE);
