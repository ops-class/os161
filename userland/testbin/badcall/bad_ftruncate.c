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
 * ftruncate
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
int
ftruncate_fd_device(void)
{
	int rv, fd;
	int result;

	report_begin("ftruncate on device");

	fd = open("null:", O_RDWR);
	if (fd<0) {
		report_warn("opening null: failed");
		report_aborted(&result);
		return result;
	}

	rv = ftruncate(fd, 6);
	result = report_check(rv, errno, EINVAL);

	close(fd);
	return result;
}

static
int
ftruncate_size_neg(void)
{
	int rv, fd;
	int result;

	report_begin("ftruncate to negative size");

	fd = open_testfile(NULL);
	if (fd<0) {
		report_aborted(&result);
		return result;
	}

	rv = ftruncate(fd, -60);
	result = report_check(rv, errno, EINVAL);

	close(fd);
	remove(TESTFILE);
	return result;
}

void
test_ftruncate(void)
{
	int ntests = 0, lost_points = 0;
	int result;

	test_ftruncate_fd(&ntests, &lost_points);

	ntests++;
	result = ftruncate_fd_device();
	handle_result(result, &lost_points);

	ntests++;
	result = ftruncate_size_neg();
	handle_result(result, &lost_points);

	if(!lost_points)
		success(TEST161_SUCCESS, SECRET, "/testbin/badcall");
}
