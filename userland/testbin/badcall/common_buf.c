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
 * Calls with invalid transfer buffers
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

static int buf_fd;

struct buftest {
	int (*setup)(void);
	int (*op)(void *);
	void (*cleanup)(void);
	const char *name;
};

////////////////////////////////////////////////////////////

static
int
read_setup(void)
{
	buf_fd = open_testfile("i do not like green eggs and ham");
	if (buf_fd<0) {
		return -1;
	}
	return 0;
}

static
int
read_badbuf(void *buf)
{
	return read(buf_fd, buf, 128);
}

static
void
read_cleanup(void)
{
	close(buf_fd);
	remove(TESTFILE);
}

//////////

static
int
write_setup(void)
{
	buf_fd = open_testfile(NULL);
	if (buf_fd<0) {
		return -1;
	}
	return 0;
}

static
int
write_badbuf(void *ptr)
{
	return write(buf_fd, ptr, 128);
}

static
void
write_cleanup(void)
{
	close(buf_fd);
	remove(TESTFILE);
}

//////////

static
int
getdirentry_setup(void)
{
	buf_fd = open(".", O_RDONLY);
	if (buf_fd < 0) {
		warn("UH-OH: couldn't open .");
		return -1;
	}
	return 0;
}

static
int
getdirentry_badbuf(void *ptr)
{
	return getdirentry(buf_fd, ptr, 1024);
}

static
void
getdirentry_cleanup(void)
{
	close(buf_fd);
}

//////////

static
int
readlink_setup(void)
{
	return create_testlink();
}

static
int
readlink_badbuf(void *buf)
{
	return readlink(TESTLINK, buf, 168);
}

static
void
readlink_cleanup(void)
{
	remove(TESTLINK);
}

//////////

static int getcwd_setup(void) { return 0; }
static void getcwd_cleanup(void) {}

static
int
getcwd_badbuf(void *buf)
{
	return __getcwd(buf, 408);
}

////////////////////////////////////////////////////////////

static
int
common_badbuf(struct buftest *info, void *buf, const char *bufdesc)
{
	int rv;
	int result;

	report_begin("%s with %s buffer", info->name, bufdesc);
	info->setup();
	rv = info->op(buf);
	result = report_check(rv, errno, EFAULT);
	info->cleanup();
	return result;
}

static
int
any_badbuf(struct buftest *info, int *ntests, int *lost_points)
{
	int result;

	*ntests += 1;
	result = common_badbuf(info, NULL, "NULL");
	handle_result(result, lost_points);

	*ntests += 1;
	result = common_badbuf(info, INVAL_PTR, "invalid");
	handle_result(result, lost_points);

	*ntests += 1;
	result = common_badbuf(info, KERN_PTR, "kernel-space");
	handle_result(result, lost_points);

	return result;
}

////////////////////////////////////////////////////////////

#define T(call) \
  void					\
  test_##call##_buf(int *ntests, int *lost_points)		\
  {					\
  	static struct buftest info = {	\
  		call##_setup,		\
  		call##_badbuf,		\
  		call##_cleanup,		\
  		#call,			\
	};				\
   	any_badbuf(&info, ntests, lost_points);	\
  }

T(read);
T(write);
T(getdirentry);
T(readlink);
T(getcwd);
