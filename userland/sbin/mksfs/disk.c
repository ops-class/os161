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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <err.h>

#include "support.h"
#include "disk.h"

#define HOSTSTRING "System/161 Disk Image"
#define BLOCKSIZE  512

#ifndef EINTR
#define EINTR 0
#endif

static int fd=-1;
static uint32_t nblocks;

/*
 * Open a disk. If we're built for the host OS, check that it's a
 * System/161 disk image, and then ignore the header block.
 */
void
opendisk(const char *path)
{
	struct stat statbuf;

	assert(fd<0);
	fd = open(path, O_RDWR);
	if (fd<0) {
		err(1, "%s", path);
	}
	if (fstat(fd, &statbuf)) {
		err(1, "%s: fstat", path);
	}

	nblocks = statbuf.st_size / BLOCKSIZE;

#ifdef HOST
	nblocks--;

	{
		char buf[64];
		int len;

		do {
			len = read(fd, buf, sizeof(buf)-1);
			if (len < 0 && (errno==EINTR || errno==EAGAIN)) {
				continue;
			}
		} while (0);

		buf[len] = 0;
		buf[strlen(HOSTSTRING)] = 0;

		if (strcmp(buf, HOSTSTRING)) {
			errx(1, "%s: Not a System/161 disk image", path);
		}
	}
#endif
}

/*
 * Return the block size. (This is fixed, but still...)
 */
uint32_t
diskblocksize(void)
{
	assert(fd>=0);
	return BLOCKSIZE;
}

/*
 * Return the device/image size in blocks.
 */
uint32_t
diskblocks(void)
{
	assert(fd>=0);
	return nblocks;
}

/*
 * Write a block.
 */
void
diskwrite(const void *data, uint32_t block)
{
	const char *cdata = data;
	uint32_t tot=0;
	int len;

	assert(fd>=0);

#ifdef HOST
	// skip over disk file header
	block++;
#endif

	if (lseek(fd, block*BLOCKSIZE, SEEK_SET)<0) {
		err(1, "lseek");
	}

	while (tot < BLOCKSIZE) {
		len = write(fd, cdata + tot, BLOCKSIZE - tot);
		if (len < 0) {
			if (errno==EINTR || errno==EAGAIN) {
				continue;
			}
			err(1, "write");
		}
		if (len==0) {
			err(1, "write returned 0?");
		}
		tot += len;
	}
}

/*
 * Read a block.
 */
void
diskread(void *data, uint32_t block)
{
	char *cdata = data;
	uint32_t tot=0;
	int len;

	assert(fd>=0);

#ifdef HOST
	// skip over disk file header
	block++;
#endif

	if (lseek(fd, block*BLOCKSIZE, SEEK_SET)<0) {
		err(1, "lseek");
	}

	while (tot < BLOCKSIZE) {
		len = read(fd, cdata + tot, BLOCKSIZE - tot);
		if (len < 0) {
			if (errno==EINTR || errno==EAGAIN) {
				continue;
			}
			err(1, "read");
		}
		if (len==0) {
			err(1, "unexpected EOF in mid-sector");
		}
		tot += len;
	}
}

/*
 * Close the disk.
 */
void
closedisk(void)
{
	assert(fd>=0);
	if (close(fd)) {
		err(1, "close");
	}
	fd = -1;
}
