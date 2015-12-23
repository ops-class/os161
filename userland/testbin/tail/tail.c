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
 * tail.c
 *
 * 	Outputs a file beginning at a specific location.
 *	Usage: tail <file> <location>
 *
 * This may be useful for testing during the file system assignment.
 */

#include <unistd.h>
#include <stdlib.h>
#include <err.h>

#define BUFSIZE 1000

/* Put buffer in data space.  We know that the program should allocate as */
/* much data space as required, but stack space is tight. */

char buffer[BUFSIZE];

static
void
tail(int file, off_t where, const char *filename)
{
	int len;

	if (lseek(file, where, SEEK_SET)<0) {
		err(1, "%s", filename);
	}

	while ((len = read(file, buffer, sizeof(buffer))) > 0) {
		write(STDOUT_FILENO, buffer, len);
	}
}

int
main(int argc, char **argv)
{
	int file;

	if (argc < 3) {
		errx(1, "Usage: tail <file> <location>");
	}
	file = open(argv[1], O_RDONLY);
	if (file < 0) {
		err(1, "%s", argv[1]);
	}
	tail(file, atoi(argv[2]), argv[1]);
	close(file);
	return 0;
}

