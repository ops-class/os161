/*
 * Copyright (c) 2014
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
 * Create a sparse file by writing one byte to the end of it.
 *
 * Should work on emufs (emu0:) once the basic system calls are done,
 * and should work on SFS when the file system assignment is
 * done. Sufficiently small files should work on SFS even before that
 * assignment.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

int
main(int argc, char *argv[])
{
	const char *filename;
	int size;
	int fd;
	int r;
	char byte;

	if (argc != 3) {
		errx(1, "Usage: sparsefile <filename> <size>");
	}

	filename = argv[1];
	size = atoi(argv[2]);
	byte = '\n';

	if (size == 0) {
		err(1, "Sparse files of length zero are not meaningful");
	}

	printf("Creating a sparse file of size %d\n", size);

	fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC);
	if (fd < 0) {
		err(1, "%s: create", filename);
	}

	if (lseek(fd, size-1, SEEK_SET) == -1) {
		err(1, "%s: lseek", filename);
	}
	r = write(fd, &byte, 1);
	if (r < 0) {
		err(1, "%s: write", filename);
	}
	else if (r != 1) {
		errx(1, "%s: write: Unexpected result count %d", filename, r);
	}

	close(fd);

	return 0;
}
