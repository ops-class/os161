/*
 * Copyright (c) 2013, 2014
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
 * poisondisk - fill a disk with nonzero poison values
 * usage: poisondisk disk-image
 */

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#ifdef HOST
#include "hostcompat.h"
#endif

#include "disk.h"

#define POISON_BYTE 0xa9
#define BLOCKSIZE 512

static
void
poison(void)
{
	char buf[BLOCKSIZE];
	off_t sectors, i;

	memset(buf, POISON_BYTE, sizeof(buf));

	sectors = diskblocks();
	for (i=0; i<sectors; i++) {
		diskwrite(buf, i);
	}
}

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		errx(1, "Usage: %s disk-image", argv[0]);
	}
	opendisk(argv[1]);
	poison();
	closedisk();
	return 0;
}
