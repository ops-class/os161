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
 * remove
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
remove_dir(void)
{
	int rv;

	report_begin("remove() on a directory");

	if (create_testdir() < 0) {
		/*report_aborted();*/ /* XXX in create_testdir */
		return;
	}

	rv = remove(TESTDIR);
	report_check(rv, errno, EISDIR);
	rmdir(TESTDIR);
}

static
void
remove_dot(void)
{
	int rv;

	report_begin("remove() on .");
	rv = remove(".");
	report_check2(rv, errno, EISDIR, EINVAL);
}

static
void
remove_dotdot(void)
{
	int rv;

	report_begin("remove() on ..");
	rv = remove("..");
	report_check2(rv, errno, EISDIR, EINVAL);
}

static
void
remove_empty(void)
{
	int rv;

	report_begin("remove() on empty string");
	rv = remove("");
	report_check2(rv, errno, EISDIR, EINVAL);
}

void
test_remove(void)
{
	test_remove_path();

	remove_dir();
	remove_dot();
	remove_dotdot();
	remove_empty();
}
