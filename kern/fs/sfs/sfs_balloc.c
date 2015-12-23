/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009, 2014
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
 * SFS filesystem
 *
 * Block allocation.
 */
#include <types.h>
#include <lib.h>
#include <bitmap.h>
#include <sfs.h>
#include "sfsprivate.h"

/*
 * Zero out a disk block.
 */
static
int
sfs_clearblock(struct sfs_fs *sfs, daddr_t block)
{
	/* static -> automatically initialized to zero */
	static char zeros[SFS_BLOCKSIZE];

	return sfs_writeblock(sfs, block, zeros, SFS_BLOCKSIZE);
}

/*
 * Allocate a block.
 */
int
sfs_balloc(struct sfs_fs *sfs, daddr_t *diskblock)
{
	int result;

	result = bitmap_alloc(sfs->sfs_freemap, diskblock);
	if (result) {
		return result;
	}
	sfs->sfs_freemapdirty = true;

	if (*diskblock >= sfs->sfs_sb.sb_nblocks) {
		panic("sfs: %s: balloc: invalid block %u\n",
		      sfs->sfs_sb.sb_volname, *diskblock);
	}

	/* Clear block before returning it */
	result = sfs_clearblock(sfs, *diskblock);
	if (result) {
		bitmap_unmark(sfs->sfs_freemap, *diskblock);
	}
	return result;
}

/*
 * Free a block.
 */
void
sfs_bfree(struct sfs_fs *sfs, daddr_t diskblock)
{
	bitmap_unmark(sfs->sfs_freemap, diskblock);
	sfs->sfs_freemapdirty = true;
}

/*
 * Check if a block is in use.
 */
int
sfs_bused(struct sfs_fs *sfs, daddr_t diskblock)
{
	if (diskblock >= sfs->sfs_sb.sb_nblocks) {
		panic("sfs: %s: sfs_bused called on out of range block %u\n",
		      sfs->sfs_sb.sb_volname, diskblock);
	}
	return bitmap_isset(sfs->sfs_freemap, diskblock);
}

