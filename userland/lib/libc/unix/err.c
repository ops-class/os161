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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

/*
 * 4.4BSD error printing functions.
 */

/*
 * This is initialized by crt0, though it actually lives in errno.c
 */
extern char **__argv;

/*
 * Routine to print error message text to stderr.
 */
static
void
__senderr(void *junk, const char *data, size_t len)
{
	(void)junk;  /* not needed or used */

	write(STDERR_FILENO, data, len);
}

/*
 * Shortcut to call __senderr on a null-terminated string.
 * (__senderr is set up to be called by __vprintf.)
 */
static
void
__senderrstr(const char *str)
{
	__senderr(NULL, str, strlen(str));
}

/*
 * Common routine for all the *err* and *warn* functions.
 */
static
void
__printerr(int use_errno, const char *fmt, va_list ap)
{
	const char *errmsg;
	const char *prog;

	/*
	 * Get the error message for the current errno.
	 * Do this early, before doing anything that might change the
	 * value in errno.
	 */
	errmsg = strerror(errno);

	/*
	 * Look up the program name.
	 * Strictly speaking we should pull off the rightmost
	 * path component of argv[0] and use that as the program
	 * name (this is how BSD err* prints) but it doesn't make
	 * much difference.
	 */
	if (__argv!=NULL && __argv[0]!=NULL) {
		prog = __argv[0];
	}
	else {
		prog = "(program name unknown)";
	}

	/* print the program name */
	__senderrstr(prog);
	__senderrstr(": ");

	/* process the printf format and args */
	__vprintf(__senderr, NULL, fmt, ap);

	/* if we're using errno, print the error string from above. */
	if (use_errno) {
		__senderrstr(": ");
		__senderrstr(errmsg);
	}

	/* and always add a newline. */
	__senderrstr("\n");
}

/*
 * The va_list versions of the warn/err functions.
 */

/* warn/vwarn: use errno, don't exit */
void
vwarn(const char *fmt, va_list ap)
{
	__printerr(1, fmt, ap);
}

/* warnx/vwarnx: don't use errno, don't exit */
void
vwarnx(const char *fmt, va_list ap)
{
	__printerr(0, fmt, ap);
}

/* err/verr: use errno, then exit */
void
verr(int exitcode, const char *fmt, va_list ap)
{
	__printerr(1, fmt, ap);
	exit(exitcode);
}

/* errx/verrx: don't use errno, but do then exit */
void
verrx(int exitcode, const char *fmt, va_list ap)
{
	__printerr(0, fmt, ap);
	exit(exitcode);
}

/*
 * The non-va_list versions of the warn/err functions.
 * Just hand off to the va_list versions.
 */

void
warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);
}

void
warnx(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vwarnx(fmt, ap);
	va_end(ap);
}

void
err(int exitcode, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verr(exitcode, fmt, ap);
	va_end(ap);
}

void
errx(int exitcode, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verrx(exitcode, fmt, ap);
	va_end(ap);
}
