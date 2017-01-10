/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009, 2015
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <assert.h>

#include "config.h"
#include "test.h"

#define RESULT_COLUMN 72

/* current screen column (0-based) */
static size_t horizpos;

/* saved screen column for subreports */
static size_t subpos;

/* pending output buffer */
static char outbuf[256];
static size_t outbufpos;

////////////////////////////////////////////////////////////

/*
 * Print things.
 */
static
void
vsay(const char *fmt, va_list ap)
{
	size_t begin, i;

	assert(outbufpos < sizeof(outbuf));

	begin = outbufpos;
	vsnprintf(outbuf + outbufpos, sizeof(outbuf) - outbufpos, fmt, ap);
	outbufpos = strlen(outbuf);

	for (i=begin; i<outbufpos; i++) {
		if (outbuf[i] == '\n') {
			horizpos = 0;
		}
		else {
			horizpos++;
		}
	}
}

static
void
say(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsay(fmt, ap);
	va_end(ap);
}

/*
 * Indent to a given horizontal position.
 */
static
void
indent_to(size_t pos)
{
	while (horizpos < pos) {
		assert(outbufpos < sizeof(outbuf) - 1);
		outbuf[outbufpos++] = ' ';
		outbuf[outbufpos] = 0;
		horizpos++;
	}
}

/*
 * Flush the output.
 */
static
void
flush(void)
{
	write(STDOUT_FILENO, outbuf, outbufpos);
	outbufpos = 0;
}

////////////////////////////////////////////////////////////

/*
 * Begin a test. This flushes the description so it can be seen before
 * the test happens, in case the test explodes or deadlocks the system.
 */
void
report_begin(const char *descfmt, ...)
{
	va_list ap;

	say("badcall: ");
	va_start(ap, descfmt);
	vsay(descfmt, ap);
	va_end(ap);
	say("... ");
	flush();
}

/*
 * Prepare to be able to print subreports.
 */
void
report_hassubs(void)
{
	subpos = horizpos;
	say("\n");
	flush();
}

/*
 * Begin a subreport. This does *not* flush because sometimes the
 * subreports are in subprocesses and we want each one to print a
 * whole line at once to avoid output interleaving.
 */
void
report_beginsub(const char *descfmt, ...)
{
	va_list ap;

	assert(horizpos == 0);
	say("   ");
	va_start(ap, descfmt);
	vsay(descfmt, ap);
	va_end(ap);
	indent_to(subpos);
}

/*
 * Print a warning message (when within a test). This generates an
 * extra line for the warning. The warnx form is the same but doesn't
 * add errno.
 */
void
report_warn(const char *fmt, ...)
{
	size_t pos;
	const char *errmsg;
	va_list ap;

	pos = horizpos;
	errmsg = strerror(errno);
	say("\n   OOPS: ");
	va_start(ap, fmt);
	vsay(fmt, ap);
	va_end(ap);
	say(": %s\n", errmsg);
	indent_to(pos);
	flush();
}

void
report_warnx(const char *fmt, ...)
{
	size_t pos;
	va_list ap;

	pos = horizpos;
	say("\n     oops: ");
	va_start(ap, fmt);
	vsay(fmt, ap);
	va_end(ap);
	say("\n");
	indent_to(pos);
	flush();
}

/*
 * Report a system call result.
 */
void
report_result(int rv, int error)
{
	if (rv == -1) {
		say("%s ", strerror(error));
	}
	else {
		say("Success ");
	}
}

/*
 * Deal with ENOSYS. The kernel prints "Unknown syscall NN\n" if you
 * call a system call that the system call dispatcher doesn't know
 * about. This is not the only way to get ENOSYS, but it's the most
 * common. So, if we see ENOSYS assume that the kernel's injected a
 * newline.
 *
 * XXX this is pretty gross.
 */
void
report_saw_enosys(void)
{
	size_t pos = horizpos;

	horizpos = 0;
	indent_to(pos);
}

/*
 * End a test. These print "passed", "FAILURE", "------", or "ABORTED"
 * in the result column, and add the final newline.
 */

static
void
report_end(const char *msg)
{
	indent_to(RESULT_COLUMN);
	say("%s\n", msg);
	flush();
}

void
report_passed(void)
{
	report_end("passed");
}

void
report_failure(void)
{
	report_end("FAILURE");
}

void
report_skipped(void)
{
	report_end("------");
}

void
report_aborted(void)
{
	report_end("ABORTED");
}

////////////////////////////////////////////////////////////

/*
 * Combination functions that call report_result and also
 * end the test, judging success based on whether the result
 * matches one or more expected results.
 */

void
report_survival(int rv, int error)
{
	/* allow any error as long as we survive */
	report_result(rv, error);
	report_passed();
}

static
void
report_checkN(int rv, int error, int *right_errors, int right_num)
{
	int i, goterror;

	if (rv==-1) {
		goterror = error;
	}
	else {
		goterror = 0;
	}

	for (i=0; i<right_num; i++) {
		if (goterror == right_errors[i]) {
			report_result(rv, error);
			report_passed();
			return;
		}
	}

	if (goterror == ENOSYS) {
		report_saw_enosys();
		say("(unimplemented) ");
		report_skipped();
	}
	else {
		report_result(rv, error);
		report_failure();
	}
}

void
report_check(int rv, int error, int right_error)
{
	report_checkN(rv, error, &right_error, 1);
}

void
report_check2(int rv, int error, int okerr1, int okerr2)
{
	int ok[2];

	ok[0] = okerr1;
	ok[1] = okerr2;
	report_checkN(rv, error, ok, 2);
}

void
report_check3(int rv, int error, int okerr1, int okerr2, int okerr3)
{
	int ok[3];

	ok[0] = okerr1;
	ok[1] = okerr2;
	ok[2] = okerr3;
	report_checkN(rv, error, ok, 3);
}
