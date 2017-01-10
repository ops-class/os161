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
 * ioctl
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "config.h"
#include "test.h"

static
int
one_ioctl_badbuf(int fd, int code, const char *codename,
		 void *ptr, const char *ptrdesc)
{
	int rv;
	int result;

	report_begin("ioctl %s with %s", codename, ptrdesc);
	rv = ioctl(fd, code, ptr);
	result = report_check(rv, errno, EFAULT);
	return result;
}

static
int
any_ioctl_badbuf(int fd, int code, const char *codename)
{
	int result;
	result = one_ioctl_badbuf(fd, code, codename, NULL, "NULL pointer");
	result |= one_ioctl_badbuf(fd, code, codename, INVAL_PTR, "invalid pointer");
	result |= one_ioctl_badbuf(fd, code, codename, KERN_PTR, "kernel pointer");
	return result;
}

#define IOCTL(fd, sym) any_ioctl_badbuf(fd, sym, #sym)

static
int
ioctl_badbuf(void)
{
	/*
	 * Since we don't actually define any ioctls, this code won't
	 * actually run. But if you do define ioctls, turn these tests
	 * on for those that actually use the data buffer argument for
	 * anything.
	 */

	/* IOCTL(STDIN_FILENO, TIOCGETA); */


	/* suppress gcc warning */
	(void)any_ioctl_badbuf;
	return 0;
}

static
int
ioctl_badcode(void)
{
	int rv;
	int result;

	report_begin("invalid ioctl");
	rv = ioctl(STDIN_FILENO, NONEXIST_IOCTL, NULL);
	result = report_check(rv, errno, EIOCTL);
	return result;
}

void
test_ioctl(void)
{
	int ntests = 0, lost_points = 0;
	int result;

	test_ioctl_fd(&ntests, &lost_points);

	/* Since we don't actually define any ioctls, this is not meaningful */
	ntests++;
	result = ioctl_badcode();
	handle_result(result, &lost_points);

	ntests++;
	result = ioctl_badbuf();
	handle_result(result, &lost_points);

	if(!lost_points)
		success(TEST161_SUCCESS, SECRET, "/testbin/badcall");
}
