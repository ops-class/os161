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
 * Block mapping logic.
 */
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <vfs.h>
#include <sfs.h>
#include "sfsprivate.h"

/*
 * Look up the disk block number (from 0 up to the number of blocks on
 * the disk) given a file and the logical block number within that
 * file. If DOALLOC is set, and no such block exists, one will be
 * allocated.
 */
int
sfs_bmap(struct sfs_vnode *sv, uint32_t fileblock, bool doalloc,
	 daddr_t *diskblock)
{
	/*
	 * I/O buffer for handling indirect blocks.
	 *
	 * Note: in real life (and when you've done the fs assignment)
	 * you would get space from the disk buffer cache for this,
	 * not use a static area.
	 */
	static uint32_t idbuf[SFS_DBPERIDB];

	struct sfs_fs *sfs = sv->sv_absvn.vn_fs->fs_data;
	daddr_t block;
	daddr_t idblock;
	uint32_t idnum, idoff;
	int result;

	KASSERT(sizeof(idbuf)==SFS_BLOCKSIZE);

	/* Since we're using a static buffer, we'd better be locked. */
	KASSERT(vfs_biglock_do_i_hold());

	/*
	 * If the block we want is one of the direct blocks...
	 */
	if (fileblock < SFS_NDIRECT) {
		/*
		 * Get the block number
		 */
		block = sv->sv_i.sfi_direct[fileblock];

		/*
		 * Do we need to allocate?
		 */
		if (block==0 && doalloc) {
			result = sfs_balloc(sfs, &block);
			if (result) {
				return result;
			}

			/* Remember what we allocated; mark inode dirty */
			sv->sv_i.sfi_direct[fileblock] = block;
			sv->sv_dirty = true;
		}

		/*
		 * Hand back the block
		 */
		if (block != 0 && !sfs_bused(sfs, block)) {
			panic("sfs: %s: Data block %u (block %u of file %u) "
			      "marked free\n", sfs->sfs_sb.sb_volname,
			      block, fileblock, sv->sv_ino);
		}
		*diskblock = block;
		return 0;
	}

	/*
	 * It's not a direct block; it must be in the indirect block.
	 * Subtract off the number of direct blocks, so FILEBLOCK is
	 * now the offset into the indirect block space.
	 */

	fileblock -= SFS_NDIRECT;

	/* Get the indirect block number and offset w/i that indirect block */
	idnum = fileblock / SFS_DBPERIDB;
	idoff = fileblock % SFS_DBPERIDB;

	/*
	 * We only have one indirect block. If the offset we were asked for
	 * is too large, we can't handle it, so fail.
	 */
	if (idnum >= SFS_NINDIRECT) {
		return EFBIG;
	}

	/* Get the disk block number of the indirect block. */
	idblock = sv->sv_i.sfi_indirect;

	if (idblock==0 && !doalloc) {
		/*
		 * There's no indirect block allocated. We weren't
		 * asked to allocate anything, so pretend the indirect
		 * block was filled with all zeros.
		 */
		*diskblock = 0;
		return 0;
	}
	else if (idblock==0) {
		/*
		 * There's no indirect block allocated, but we need to
		 * allocate a block whose number needs to be stored in
		 * the indirect block. Thus, we need to allocate an
		 * indirect block.
		 */
		result = sfs_balloc(sfs, &idblock);
		if (result) {
			return result;
		}

		/* Remember the block we just allocated */
		sv->sv_i.sfi_indirect = idblock;

		/* Mark the inode dirty */
		sv->sv_dirty = true;

		/* Clear the indirect block buffer */
		bzero(idbuf, sizeof(idbuf));
	}
	else {
		/*
		 * We already have an indirect block allocated; load it.
		 */
		result = sfs_readblock(sfs, idblock, idbuf, sizeof(idbuf));
		if (result) {
			return result;
		}
	}

	/* Get the block out of the indirect block buffer */
	block = idbuf[idoff];

	/* If there's no block there, allocate one */
	if (block==0 && doalloc) {
		result = sfs_balloc(sfs, &block);
		if (result) {
			return result;
		}

		/* Remember the block we allocated */
		idbuf[idoff] = block;

		/* The indirect block is now dirty; write it back */
		result = sfs_writeblock(sfs, idblock, idbuf, sizeof(idbuf));
		if (result) {
			return result;
		}
	}

	/* Hand back the result and return. */
	if (block != 0 && !sfs_bused(sfs, block)) {
		panic("sfs: %s: Data block %u (block %u of file %u) "
		      "marked free\n", sfs->sfs_sb.sb_volname,
		      block, fileblock, sv->sv_ino);
	}
	*diskblock = block;
	return 0;
}

/*
 * Called for ftruncate() and from sfs_reclaim.
 */
int
sfs_itrunc(struct sfs_vnode *sv, off_t len)
{
	/*
	 * I/O buffer for handling the indirect block.
	 *
	 * Note: in real life (and when you've done the fs assignment)
	 * you would get space from the disk buffer cache for this,
	 * not use a static area.
	 */
	static uint32_t idbuf[SFS_DBPERIDB];

	struct sfs_fs *sfs = sv->sv_absvn.vn_fs->fs_data;

	/* Length in blocks (divide rounding up) */
	uint32_t blocklen = DIVROUNDUP(len, SFS_BLOCKSIZE);

	uint32_t i, j;
	daddr_t block, idblock;
	uint32_t baseblock, highblock;
	int result;
	int hasnonzero, iddirty;

	KASSERT(sizeof(idbuf)==SFS_BLOCKSIZE);

	vfs_biglock_acquire();

	/*
	 * Go through the direct blocks. Discard any that are
	 * past the limit we're truncating to.
	 */
	for (i=0; i<SFS_NDIRECT; i++) {
		block = sv->sv_i.sfi_direct[i];
		if (i >= blocklen && block != 0) {
			sfs_bfree(sfs, block);
			sv->sv_i.sfi_direct[i] = 0;
			sv->sv_dirty = true;
		}
	}

	/* Indirect block number */
	idblock = sv->sv_i.sfi_indirect;

	/* The lowest block in the indirect block */
	baseblock = SFS_NDIRECT;

	/* The highest block in the indirect block */
	highblock = baseblock + SFS_DBPERIDB - 1;

	if (blocklen < highblock && idblock != 0) {
		/* We're past the proposed EOF; may need to free stuff */

		/* Read the indirect block */
		result = sfs_readblock(sfs, idblock, idbuf, sizeof(idbuf));
		if (result) {
			vfs_biglock_release();
			return result;
		}

		hasnonzero = 0;
		iddirty = 0;
		for (j=0; j<SFS_DBPERIDB; j++) {
			/* Discard any blocks that are past the new EOF */
			if (blocklen < baseblock+j && idbuf[j] != 0) {
				sfs_bfree(sfs, idbuf[j]);
				idbuf[j] = 0;
				iddirty = 1;
			}
			/* Remember if we see any nonzero blocks in here */
			if (idbuf[j]!=0) {
				hasnonzero=1;
			}
		}

		if (!hasnonzero) {
			/* The whole indirect block is empty now; free it */
			sfs_bfree(sfs, idblock);
			sv->sv_i.sfi_indirect = 0;
			sv->sv_dirty = true;
		}
		else if (iddirty) {
			/* The indirect block is dirty; write it back */
			result = sfs_writeblock(sfs, idblock, idbuf,
						sizeof(idbuf));
			if (result) {
				vfs_biglock_release();
				return result;
			}
		}
	}

	/* Set the file size */
	sv->sv_i.sfi_size = len;

	/* Mark the inode dirty */
	sv->sv_dirty = true;

	vfs_biglock_release();
	return 0;
}

