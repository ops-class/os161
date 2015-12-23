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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

/*
 * POSIX C function: exec a program on the search path. Tries
 * execv() repeatedly until one of the choices works.
 */
int
execvp(const char *prog, char *const *args)
{
	const char *searchpath, *s, *t;
	char progpath[PATH_MAX];
	size_t len;

	if (strchr(prog, '/') != NULL) {
		execv(prog, args);
		return -1;
	}

	searchpath = getenv("PATH");
	if (searchpath == NULL) {
		errno = ENOENT;
		return -1;
	}

	for (s = searchpath; s != NULL; s = t) {
		t = strchr(s, ':');
		if (t != NULL) {
			len = t - s;
			/* advance past the colon */
			t++;
		}
		else {
			len = strlen(s);
		}
		if (len == 0) {
			continue;
		}
		if (len >= sizeof(progpath)) {
			continue;
		}
		memcpy(progpath, s, len);
		snprintf(progpath + len, sizeof(progpath) - len, "/%s", prog);
		execv(progpath, args);
		switch (errno) {
		    case ENOENT:
		    case ENOTDIR:
		    case ENOEXEC:
			/* routine errors, try next dir */
			break;
		    default:
			/* oops, let's fail */
			return -1;
		}
	}
	errno = ENOENT;
	return -1;
}
