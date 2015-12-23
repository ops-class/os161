/*
 * Copyright (c) 2013
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

#include <stdlib.h>
#include <string.h>

/*
 * getenv(): ANSI C
 *
 * Get an environment variable.
 */

/*
 * This is initialized by crt0, though it actually lives in errno.c
 */
extern char **__environ;

/*
 * This is what we use by default if the kernel didn't supply an
 * environment.
 */
static const char *__default_environ[] = {
	"PATH=/bin:/sbin:/testbin",
	"SHELL=/bin/sh",
	"TERM=vt220",
	NULL
};

char *
getenv(const char *var)
{
	size_t varlen, thislen;
	char *s;
	unsigned i;

	if (__environ == NULL) {
		__environ = (char **)__default_environ;
	}
	varlen = strlen(var);
	for (i=0; __environ[i] != NULL; i++) {
		s = strchr(__environ[i], '=');
		if (s == NULL) {
			/* ? */
			continue;
		}
		thislen = s - __environ[i];
		if (thislen == varlen && !memcmp(__environ[i], var, thislen)) {
			return s + 1;
		}
	}
	return NULL;
}
