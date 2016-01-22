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
 * f_read.c
 *
 *	This used to be a separate binary, because it came from Nachos
 *	and nachos didn't support fork(). However, in OS/161 there's
 *	no reason to make it a separate binary; doing so just makes
 *	the test flaky.
 *
 *
 * 	it will start reading from a given file, concurrently
 * 	with other instances of f_read and f_write.
 *
 */

#define SectorSize   512

#define TMULT        50
#define FSIZE        ((SectorSize + 1) * TMULT)

#define FNAME        "f-testfile"
#define READCHAR     'r'
#define WRITECHAR    'w'

#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include "f_hdr.h"

static char buffer[SectorSize + 1];

static
void
check_buffer(void)
{
	int i;
	char ch = buffer[0];

	for (i = 1; i < SectorSize + 1; i++) {
		if (buffer[i] != ch) {
			errx(1, "Read error: %s", buffer);
		}
	}

	putchar(ch);
}

void
subproc_read(void)
{
	int fd;
	int i, res;

	tprintf("File Reader starting ...\n\n");

	fd = open(FNAME, O_RDONLY);
	if (fd < 0) {
		err(1, "%s: open", FNAME);
	}

	for (i=0; i<TMULT; i++) {
		res = read(fd, buffer, SectorSize + 1);
		if (res < 0) {
			err(1, "%s: read", FNAME);
		}

		// yield();

		if (res != SectorSize + 1) {
			errx(1, "%s: read: short count", FNAME);
		}
		check_buffer();
	}

	close(fd);

	tprintf("File Read exited successfully!\n");
}
