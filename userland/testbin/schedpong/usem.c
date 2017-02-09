/*
 * Copyright (c) 2014, 2015
 *	The President and Fellows of Harvard College.
 *      Written by David A. Holland.
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
 * XXX this code is mostly copied from usemtest; it should go in a
 * library.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "usem.h"

void
usem_init(struct usem *sem, const char *namefmt, ...)
{
	va_list ap;

	va_start(ap, namefmt);
	vsnprintf(sem->name, sizeof(sem->name), namefmt, ap);
	va_end(ap);

	sem->fd = open(sem->name, O_RDWR|O_CREAT|O_TRUNC, 0664);
	if (sem->fd < 0) {
		err(1, "%s: create", sem->name);
	}
	close(sem->fd);
	sem->fd = -1;
}

void
usem_open(struct usem *sem)
{
	sem->fd = open(sem->name, O_RDWR);
	if (sem->fd < 0) {
		err(1, "%s: open", sem->name);
	}
}

void
usem_close(struct usem *sem)
{
	if (close(sem->fd) == -1) {
		warn("%s: close", sem->name);
	}
}

void
usem_cleanup(struct usem *sem)
{
	(void)remove(sem->name);
}

void
Pn(struct usem *sem, unsigned count)
{
	ssize_t r;
	char c[count];

	r = read(sem->fd, c, count);
	if (r < 0) {
		err(1, "%s: read", sem->name);
	}
	if ((size_t)r < count) {
		errx(1, "%s: read: unexpected EOF", sem->name);
	}
}

void
P(struct usem *sem)
{
	Pn(sem, 1);
}

void
Vn(struct usem *sem, unsigned count)
{
	ssize_t r;
	char c[count];

	/* semfs does not use these values, but be conservative */
	memset(c, 0, count);

	r = write(sem->fd, c, count);
	if (r < 0) {
		err(1, "%s: write", sem->name);
	}
	if ((size_t)r < count) {
		errx(1, "%s: write: Short count", sem->name);
	}
}

void
V(struct usem *sem)
{
	Vn(sem, 1);
}
