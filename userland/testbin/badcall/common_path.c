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
 * Calls with invalid pathnames
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
int
open_badpath(const char *path)
{
	return open(path, O_RDONLY);
}

static
int
remove_badpath(const char *path)
{
	return remove(path);
}

static
int
rename_badpath1(const char *path)
{
	return rename(path, TESTFILE);
}

static
int
rename_badpath2(const char *path)
{
	return rename(TESTFILE, path);
}

static
int
link_badpath1(const char *path)
{
	return link(path, TESTFILE);
}

static
int
link_badpath2(const char *path)
{
	return link(TESTFILE, path);
}

static
int
mkdir_badpath(const char *path)
{
	return mkdir(path, 0775);
}

static
int
rmdir_badpath(const char *path)
{
	return rmdir(path);
}

static
int
chdir_badpath(const char *path)
{
	return chdir(path);
}

static
int
symlink_badpath1(const char *path)
{
	return symlink(path, TESTFILE);
}

static
int
symlink_badpath2(const char *path)
{
	return symlink(TESTFILE, path);
}

static
int
readlink_badpath(const char *path)
{
	char buf[128];
	return readlink(path, buf, sizeof(buf));
}

static
int
lstat_badpath(const char *name)
{
	struct stat sb;
	return lstat(name, &sb);
}

static
int
stat_badpath(const char *name)
{
	struct stat sb;
	return stat(name, &sb);
}

////////////////////////////////////////////////////////////

static
int
common_badpath(int (*func)(const char *path), int mk, int rm, const char *path,
	       const char *call, const char *pathdesc)
{
	int rv;
	int result;

	report_begin("%s with %s path", call, pathdesc);

	if (mk) {
		if (create_testfile()<0) {
			report_aborted(&result);
			return result;
		}
	}

	rv = func(path);
	result = report_check(rv, errno, EFAULT);

	if (mk || rm) {
		remove(TESTFILE);
	}
	return result;
}

static
void
any_badpath(int (*func)(const char *path), const char *call, int mk, int rm,
		int *ntests, int *lost_points)
{
	int result;
	// We have a total of 3 tests
	*ntests = *ntests + 3;

	result = common_badpath(func, mk, rm, NULL, call, "NULL");
	handle_result(result, lost_points);

	result = common_badpath(func, mk, rm, INVAL_PTR, call, "invalid-pointer");
	handle_result(result, lost_points);

	result = common_badpath(func, mk, rm, KERN_PTR, call, "kernel-pointer");
	handle_result(result, lost_points);
}

////////////////////////////////////////////////////////////

/* functions with one pathname */
#define T(call)                                                        \
  void                                                                 \
  test_##call##_path(int *ntests, int *lost_points)                    \
  {                                                                    \
   	any_badpath(call##_badpath, #call, 0, 0, ntests, lost_points); \
  }

T(open);
T(remove);
T(mkdir);
T(rmdir);
T(chdir);
T(readlink);
T(stat);
T(lstat);

/* functions with two pathnames */
#define T2(call)                                                                 \
  void                                                                           \
  test_##call##_paths(int *ntests, int *lost_points)                             \
  {                                                                              \
   	any_badpath(call##_badpath1, #call "(arg1)", 0, 1, ntests, lost_points); \
   	any_badpath(call##_badpath2, #call "(arg2)", 1, 1, ntests, lost_points); \
  }

T2(rename);
T2(link);
T2(symlink);
