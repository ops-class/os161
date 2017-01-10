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
 * Create a large file in small increments.
 *
 * Should work on emufs (emu0:) once the basic system calls are done,
 * and should work on SFS when the file system assignment is
 * done. Sufficiently small files should work on SFS even before that
 * assignment.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

static char buffer[8192 + 1];

int
main(int argc, char *argv[])
{
	const char *filename;
	char *s;
	size_t i, size, chunksize, offset;
	ssize_t len;
	int fd;

	if (argc != 3) {
		warnx("Usage: bigfile <filename> <size>");
		errx(1, "   or: bigfile <filename> <size>/<chunksize>");
	}

	filename = argv[1];
	s = strchr(argv[2], '/');
	if (s != NULL) {
		*s++ = 0;
		chunksize = atoi(s);
		if (chunksize >= sizeof(buffer)) {
			chunksize = sizeof(buffer) - 1;
		}
		if (chunksize == 0) {
			errx(1, "Really?");
		}
	}
	else {
		chunksize = 10;
	}
	size = atoi(argv[2]);

	/* round size up */
	size = ((size + chunksize - 1) / chunksize) * chunksize;

	tprintf("Creating a file of size %d in %d-byte chunks\n",
	       size, chunksize);

	fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC);
	if (fd < 0) {
		err(1, "%s: create", filename);
	}

	i=0;
	while (i<size) {
		snprintf(buffer, sizeof(buffer), "%d\n", i);
		if (strlen(buffer) < chunksize) {
			offset = chunksize - strlen(buffer);
			memmove(buffer + offset, buffer, strlen(buffer)+1);
			memset(buffer, ' ', offset);
		}
		len = write(fd, buffer, strlen(buffer));
		if (len<0) {
			err(1, "%s: write", filename);
		}
		i += len;
	}

	close(fd);

	return 0;
}
