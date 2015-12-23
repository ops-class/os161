/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2009, 2013, 2014
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

#include <sys/types.h>	/* for CHAR_BIT */
#include <limits.h>	/* also for CHAR_BIT */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <err.h>

#include "compat.h"
#include <kern/sfs.h>

#include "utils.h"
#include "sfs.h"
#include "sb.h"
#include "freemap.h"
#include "main.h"

static unsigned long blocksinuse = 0;
static uint8_t *freemapdata;
static uint8_t *tofreedata;

/*
 * Allocate space to keep track of the free block bitmap. This is
 * called after the superblock is loaded so we can ask how big the
 * volume is.
 */
void
freemap_setup(void)
{
	size_t i, mapbytes;
	uint32_t fsblocks, mapblocks;

	fsblocks = sb_totalblocks();
	mapblocks = sb_freemapblocks();
	mapbytes = mapblocks * SFS_BLOCKSIZE;

	freemapdata = domalloc(mapbytes * sizeof(uint8_t));
	tofreedata = domalloc(mapbytes * sizeof(uint8_t));
	for (i=0; i<mapbytes; i++) {
		freemapdata[i] = tofreedata[i] = 0;
	}

	/* Mark off what's in the freemap but past the volume end. */
	for (i=fsblocks; i < mapblocks*SFS_BITSPERBLOCK; i++) {
		freemap_blockinuse(i, B_PASTEND, 0);
	}

	/* Mark the superblock block and the freemap blocks in use */
	freemap_blockinuse(SFS_SUPER_BLOCK, B_SUPERBLOCK, 0);
	for (i=0; i < mapblocks; i++) {
		freemap_blockinuse(SFS_FREEMAP_START+i, B_FREEMAPBLOCK, i);
	}
}

/*
 * Return a string for a blockusage; used for printing errors.
 */
static
const char *
blockusagestr(blockusage_t how, uint32_t howdesc)
{
	static char rv[256];
	switch (how) {
	    case B_SUPERBLOCK:
		return "superblock";
	    case B_FREEMAPBLOCK:
		snprintf(rv, sizeof(rv), "freemap block %lu",
			 (unsigned long) howdesc);
		break;
	    case B_INODE:
		snprintf(rv, sizeof(rv), "inode %lu",
			 (unsigned long) howdesc);
		break;
	    case B_IBLOCK:
		snprintf(rv, sizeof(rv), "indirect block of inode %lu",
			 (unsigned long) howdesc);
		break;
	    case B_DIRDATA:
		snprintf(rv, sizeof(rv), "directory data from inode %lu",
			 (unsigned long) howdesc);
		break;
	    case B_DATA:
		snprintf(rv, sizeof(rv), "file data from inode %lu",
			 (unsigned long) howdesc);
		break;
	    case B_PASTEND:
		return "past the end of the fs";
	}
	return rv;
}

/*
 * Mark block BLOCK in use. HOW and HOWDESC describe how it was found
 * to be in use, so we can print a useful message if it's wrong.
 *
 * FUTURE: this should not produce unrecoverable errors.
 */
void
freemap_blockinuse(uint32_t block, blockusage_t how, uint32_t howdesc)
{
	unsigned index = block/8;
	uint8_t mask = ((uint8_t)1)<<(block%8);

	if (tofreedata[index] & mask) {
		/* really using the block, don't free it */
		tofreedata[index] &= ~mask;
	}

	if (freemapdata[index] & mask) {
		warnx("Block %lu (used as %s) already in use! (NOT FIXED)",
		      (unsigned long) block, blockusagestr(how, howdesc));
		setbadness(EXIT_UNRECOV);
	}

	freemapdata[index] |= mask;

	if (how != B_PASTEND) {
		blocksinuse++;
	}
}

/*
 * Mark a block free. This is specifically for blocks that we are
 * freeing, that might be marked allocated in the on-disk freemap. If
 * the block has been found in use, assume the reference that's in use
 * is valid. This can be caused by freeing a block, reallocating it
 * somewhere else and then dying without erasing all of the original
 * usage on disk; most such cases will just show the block in use
 * twice, which is (not) handled above, but it's possible for the
 * original usage to be something we are dropping, e.g. if a truncate
 * (to a nonzero length > INOMAX_D) got partially completed.
 */
void
freemap_blockfree(uint32_t block)
{
	unsigned index = block/8;
	uint8_t mask = ((uint8_t)1)<<(block%8);

	if (tofreedata[index] & mask) {
		/* already marked to free once, ignore */
		return;
	}
	if (freemapdata[index] & mask) {
		/* block is used elsewhere, ignore */
		return;
	}
	tofreedata[index] |= mask;
}

/*
 * Count the number of bits set.
 */
static
int
countbits(uint8_t val)
{
	uint8_t x;
	int ct=0;

	for (x=1; x; x<<=1) {
		if (val & x) ct++;
	}
	return ct;
}

/*
 * Print a complaint about freemap bits being wrong.
 *
 * FREEMAPBLOCK is the block number within the freemap; BYTE is the
 * byte offset within that block; VAL is the byte value; WHAT is a
 * string indicating what happened.
 */
static
void
reportfreemap(uint32_t mapblock, uint32_t byte, uint8_t val, const char *what)
{
	uint8_t x, y;
	uint32_t blocknum;

	for (x=1, y=0; x; x<<=1, y++) {
		if (val & x) {
			blocknum = mapblock*SFS_BITSPERBLOCK +
				byte*CHAR_BIT + y;
			warnx("Block %lu erroneously shown %s in freemap",
			      (unsigned long) blocknum, what);
		}
	}
}

/*
 * Scan the freemap.
 *
 * This is called after (at the end of) pass 1, when we've recursively
 * found all the reachable blocks and marked them.
 */
void
freemap_check(void)
{
	uint8_t actual[SFS_BLOCKSIZE], *expected, *tofree, tmp;
	uint32_t alloccount=0, freecount=0, i, j;
	int bchanged;
	uint32_t bitblocks;

	bitblocks = sb_freemapblocks();

	for (i=0; i<bitblocks; i++) {
		sfs_readfreemapblock(i, actual);
		expected = freemapdata + i*SFS_BLOCKSIZE;
		tofree = tofreedata + i*SFS_BLOCKSIZE;
		bchanged = 0;

		for (j=0; j<SFS_BLOCKSIZE; j++) {
			/* we shouldn't have blocks marked both ways */
			assert((expected[j] & tofree[j])==0);

			/* what's there is what should be there */
			if (actual[j] == expected[j]) {
				continue;
			}

			/* what's there is what should be there modulo frees */
			if (actual[j] == (expected[j] | tofree[j])) {
				actual[j] = expected[j];
				bchanged = 1;
				continue;
			}

			/* oops, it doesn't match... */

			/* free the ones we're freeing (don't report these) */
			actual[j] &= ~tofree[j];

			/* are we short any? */
			if ((actual[j] & expected[j]) != expected[j]) {
				tmp = expected[j] & ~actual[j];
				alloccount += countbits(tmp);
				if (tmp != 0) {
					reportfreemap(i, j, tmp, "free");
				}
			}

			/* do we have any extra? */
			if ((actual[j] & expected[j]) != actual[j]) {
				tmp = actual[j] & ~expected[j];
				freecount += countbits(tmp);
				if (tmp != 0) {
					reportfreemap(i, j, tmp, "allocated");
				}
			}

			/* set it to what it should be */
			actual[j] = expected[j];
			bchanged = 1;
		}

		/* write the block back if necessary */
		if (bchanged) {
			sfs_writefreemapblock(i, actual);
		}
	}

	if (alloccount > 0) {
		warnx("%lu blocks erroneously shown free in freemap (fixed)",
		      (unsigned long) alloccount);
		setbadness(EXIT_RECOV);
	}
	if (freecount > 0) {
		warnx("%lu blocks erroneously shown used in freemap (fixed)",
		      (unsigned long) freecount);
		setbadness(EXIT_RECOV);
	}
}

/*
 * Return the total number of blocks in use, which we count during
 * pass 1.
 */
unsigned long
freemap_blocksused(void)
{
	return blocksinuse;
}
