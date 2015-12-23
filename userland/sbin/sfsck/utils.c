/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2009, 2013
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "compat.h"
#include "utils.h"
#include "main.h"

/*
 * Wrapper around malloc.
 */
void *
domalloc(size_t len)
{
	void *x;
	x = malloc(len);
	if (x==NULL) {
		errx(EXIT_FATAL, "Out of memory");
	}
	return x;
}

/*
 * Wrapper around realloc. OSZ is the old block size, which we need if
 * we're going to emulate realloc with malloc.
 */
void *
dorealloc(void *op, size_t osz, size_t nsz)
{
	void *np;
#ifdef NO_REALLOC
	size_t copysz;

	np = domalloc(nsz);
	if (op != NULL) {
		copysz = osz < nsz ? osz : nsz;
		memcpy(np, op, copysz);
		free(op);
	}
#else
	(void)osz;
	np = realloc(op, nsz);
	if (np == NULL) {
		errx(EXIT_FATAL, "Out of memory");
	}
#endif
	return np;
}

/*
 * Get a unique id number. (unique as in for this run of sfsck...)
 */
uint32_t
uniqueid(void)
{
	static uint32_t uniquecounter;

	return uniquecounter++;
}

/*
 * Check if BUF, a string field of length MAXLEN, contains a null
 * terminator. If not, slam one in and return 1.
 */
int
checknullstring(char *buf, size_t maxlen)
{
	size_t i;
	for (i=0; i<maxlen; i++) {
		if (buf[i]==0) {
			return 0;
		}
	}
	buf[maxlen-1] = 0;
	return 1;
}

/*
 * Check if BUF contains characters not allowed in file and volume
 * names. If so, stomp them and return 1.
 */
int
checkbadstring(char *buf)
{
	size_t i;
	int rv = 0;

	for (i=0; buf[i]; i++) {
		if (buf[i]==':' || buf[i]=='/') {
			buf[i] = '_';
			rv = 1;
		}
	}
	return rv;
}

/*
 * Check if BUF, of size LEN, is zeroed. If not, zero it and return 1.
 */
int
checkzeroed(void *vbuf, size_t len)
{
	char *buf = vbuf;
	size_t i;
	int rv = 0;

	for (i=0; i < len; i++) {
		if (buf[i] != 0) {
			buf[i] = 0;
			rv = 1;
		}
	}
	return rv;
}
