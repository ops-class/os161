/*
 * Copyright (c) 2013
 *	The President and Fellows of Harvard College.
 *      Written by David A. Holland.
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
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "data.h"

/*
 * XXX for now hardwire things we know about SFS
 */
#define BLOCKSIZE /*SFS_BLOCKSIZE*/ 512

static char databuf[DATA_MAXSIZE];
static char readbuf[DATA_MAXSIZE];

static
void
prepdata(unsigned code, unsigned seq, char *buf, off_t len)
{
	char smallbuf[32];
	char letter;
	size_t slen;

	snprintf(smallbuf, sizeof(smallbuf), "%u@%u\n", seq, code);
	slen = strlen(smallbuf);

	while (len >= slen) {
		memcpy(buf, smallbuf, slen);
		buf += slen;
		len -= slen;
	}
	if (len > 1) {
		letter = 'A' + (code + seq) % 26;
		memset(buf, letter, len - 1);
		buf += len - 1;
	}
	if (len > 0) {
		*buf = '\n';
	}
}

static
int
matches_at(size_t start, size_t len)
{
	if (!memcmp(databuf + start, readbuf + start, len)) {
		return 1;
	}
	return 0;
}

static
int
byte_at(size_t start, size_t len, unsigned char val)
{
	size_t i;

	for (i=0; i<len; i++) {
		if ((unsigned char)readbuf[start + i] != val) {
			return 0;
		}
	}
	return 1;
}

static
int
zero_at(size_t start, size_t len)
{
	return byte_at(start, len, 0);
}

static
int
poison_at(size_t start, size_t len)
{
	return byte_at(start, len, POISON_VAL);
}

/*
 * Check if the data found in the read buffer matches what should be
 * there.
 *    NAMESTR is the name of the file, for error reporting.
 *    REGIONOFFSET is where the write region began.
 *    CODE and SEQ are the keys for generating the expected data.
 *    ZEROSTART is the first offset into the write region at which the
 *        data "can" be zeros instead.
 *    LEN is the length of the write.
 *    CHECKSTART is the offset into the write region where we begin checking.
 *    CHECKLEN is the length of the region we check.
 */
int
data_matches(const char *namestr, off_t regionoffset,
	     unsigned code, unsigned seq, off_t zerostart, off_t len,
	     off_t checkstart, off_t checklen)
{
	int ret;
	off_t where;
	size_t howmuch;
	off_t absend, slop;

	assert(len <= DATA_MAXSIZE);
	assert(checklen > 0);
	assert(checklen <= len);
	assert(checkstart >= 0 && checkstart < len);
	assert(checkstart + checklen <= len);
	assert(zerostart >= 0);
	assert(zerostart <= len);

	prepdata(code, seq, databuf, len);

	ret = 1;
	while (checklen > 0) {
		/* check one block at a time */
		where = checkstart;
		howmuch = BLOCKSIZE;
		/* no more than is left to do */
		if (howmuch > checklen) {
			howmuch = checklen;
		}
		/* if we stick over a block boundary, stop there */
		absend = regionoffset + where + howmuch;
		slop = absend % BLOCKSIZE;
		if (slop != 0 && slop < howmuch) {
			howmuch -= slop;
		}
		/* if we go past the zerostart point, stop there */
		if (where < zerostart && where + howmuch > zerostart) {
			howmuch = zerostart - where;
		}
		assert(howmuch > 0);

		if (matches_at(where, howmuch)) {
			/* nothing */
		}
		else if (zero_at(where, howmuch)) {
			if (where >= zerostart) {
				printf("WARNING: file %s range %lld-%lld is "
				       "zeroed\n",
				       namestr, regionoffset + where,
				       regionoffset + where + howmuch);
			}
			else {
				ret = 0;
			}
		}
		else if (poison_at(where, howmuch)) {
			if (where >= zerostart) {
				printf("ERROR: file %s range %lld-%lld is "
				       "poisoned\n",
				       namestr, regionoffset + where,
				       regionoffset + where + howmuch);
			}
			else {
				ret = 0;
			}
		}
		else {
			ret = 0;
		}

		checkstart += howmuch;
		checklen -= howmuch;
	}
	return ret;
}

void
data_check(const char *namestr, off_t regionoffset,
	   unsigned code, unsigned seq, off_t zerostart, off_t len,
	   off_t checkstart, off_t checklen)
{
	assert(zerostart >= 0);
	assert(zerostart <= len);

	if (!data_matches(namestr, regionoffset,
			  code, seq, zerostart, len, checkstart, checklen)) {
		printf("ERROR: file %s range %lld-%lld contains garbage\n",
		       namestr, regionoffset + checkstart,
		       regionoffset + checkstart + checklen);
	}
}

void *
data_map(unsigned code, unsigned seq, off_t len)
{
	assert(len <= DATA_MAXSIZE);
	prepdata(code, seq, databuf, len);
	return databuf;
}

void *
data_mapreadbuf(off_t len)
{
	assert(len <= DATA_MAXSIZE);
	return readbuf;
}
