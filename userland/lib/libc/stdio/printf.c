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

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <kern/secret.h>

/*
 * printf - C standard I/O function.
 */


/*
 * Function passed to __vprintf to do the actual output.
 */
static
void
__printf_send(void *mydata, const char *data, size_t len)
{
	ssize_t ret;
	int *err = mydata;

	ret = write(STDOUT_FILENO, data, len);
	*err = (ret == -1) ? errno : 0;
}

/* printf: hand off to vprintf */
int
printf(const char *fmt, ...)
{
	int chars;
	va_list ap;

	va_start(ap, fmt);
	chars = vprintf(fmt, ap);
	va_end(ap);
	return chars;
}

/* vprintf: call __vprintf to do the work. */
int
vprintf(const char *fmt, va_list ap)
{
	int chars, err;
	chars = __vprintf(__printf_send, &err, fmt, ap);
	if (err) {
		errno = err;
		return -1;
	}
	return chars;
}
