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
#include <string.h>
#include <test161/test161.h>

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
	byte = '@';

	if (size == 0) {
		err(1, "Sparse files of length zero are not meaningful");
	}

	tprintf("Creating a sparse file of size %d\n", size);
	nprintf(".");

	fd = open(filename, O_RDWR|O_CREAT|O_TRUNC);
	if (fd < 0) {
		err(1, "%s: create", filename);
	}

	if (lseek(fd, size-1, SEEK_SET) == -1) {
		err(1, "%s: lseek", filename);
	}
	nprintf(".");
	r = write(fd, &byte, 1);
	if (r < 0) {
		err(1, "%s: write", filename);
	}
	else if (r != 1) {
		errx(1, "%s: write: Unexpected result count %d", filename, r);
	}
	nprintf(".");

	// Now check this byte.
	// First seek to the beginning and then seek back to where the byte
	// should be.
	if(lseek(fd, 0, SEEK_SET) == -1) {
		err(1, "lseek failed to seek to beginning of file\n");
	}
	nprintf(".");
	// Now seek back to where the byte should be
	// While at it, also test SEEK_CUR
	if(lseek(fd, size-1, SEEK_CUR) == -1) {
		err(1, "lseek failed to seek to %d of file\n", size-1);
	}
	nprintf(".");
	char test;
	r = read(fd, &test, 1);
	if(test != byte) {
		err(1, "Byte test failed. Expected (%c) != Observed (%c)\n", byte, test);
	}
	nprintf(".");
	close(fd);

	nprintf("\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/sparsefile");
	return 0;
}
