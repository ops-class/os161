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
 * I/O plumbing.
 */
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <uio.h>
#include <vfs.h>
#include <device.h>
#include <sfs.h>
#include "sfsprivate.h"

////////////////////////////////////////////////////////////
//
// Basic block-level I/O routines

/*
 * Note: sfs_readblock is used to read the superblock
 * early in mount, before sfs is fully (or even mostly)
 * initialized, and so may not use anything from sfs
 * except sfs_device.
 */

/*
 * Read or write a block, retrying I/O errors.
 */
static
int
sfs_rwblock(struct sfs_fs *sfs, struct uio *uio)
{
	int result;
	int tries=0;

	KASSERT(vfs_biglock_do_i_hold());

	DEBUG(DB_SFS, "sfs: %s %llu\n",
	      uio->uio_rw == UIO_READ ? "read" : "write",
	      uio->uio_offset / SFS_BLOCKSIZE);

 retry:
	result = DEVOP_IO(sfs->sfs_device, uio);
	if (result == EINVAL) {
		/*
		 * This means the sector we requested was out of range,
		 * or the seek address we gave wasn't sector-aligned,
		 * or a couple of other things that are our fault.
		 */
		panic("sfs: %s: DEVOP_IO returned EINVAL\n",
		      sfs->sfs_sb.sb_volname);
	}
	if (result == EIO) {
		if (tries == 0) {
			tries++;
			kprintf("sfs: %s: block %llu I/O error, retrying\n",
				sfs->sfs_sb.sb_volname,
				uio->uio_offset / SFS_BLOCKSIZE);
			goto retry;
		}
		else if (tries < 10) {
			tries++;
			goto retry;
		}
		else {
			kprintf("sfs: %s: block %llu I/O error, giving up "
				"after %d retries\n",
				sfs->sfs_sb.sb_volname,
				uio->uio_offset / SFS_BLOCKSIZE, tries);
		}
	}
	return result;
}

/*
 * Read a block.
 */
int
sfs_readblock(struct sfs_fs *sfs, daddr_t block, void *data, size_t len)
{
	struct iovec iov;
	struct uio ku;

	KASSERT(len == SFS_BLOCKSIZE);

	SFSUIO(&iov, &ku, data, block, UIO_READ);
	return sfs_rwblock(sfs, &ku);
}

/*
 * Write a block.
 */
int
sfs_writeblock(struct sfs_fs *sfs, daddr_t block, void *data, size_t len)
{
	struct iovec iov;
	struct uio ku;

	KASSERT(len == SFS_BLOCKSIZE);

	SFSUIO(&iov, &ku, data, block, UIO_WRITE);
	return sfs_rwblock(sfs, &ku);
}

////////////////////////////////////////////////////////////
//
// File-level I/O

/*
 * Do I/O to a block of a file that doesn't cover the whole block.  We
 * need to read in the original block first, even if we're writing, so
 * we don't clobber the portion of the block we're not intending to
 * write over.
 *
 * SKIPSTART is the number of bytes to skip past at the beginning of
 * the sector; LEN is the number of bytes to actually read or write.
 * UIO is the area to do the I/O into.
 */
static
int
sfs_partialio(struct sfs_vnode *sv, struct uio *uio,
	      uint32_t skipstart, uint32_t len)
{
	/*
	 * I/O buffer for handling partial sectors.
	 *
	 * Note: in real life (and when you've done the fs assignment)
	 * you would get space from the disk buffer cache for this,
	 * not use a static area.
	 */
	static char iobuf[SFS_BLOCKSIZE];

	struct sfs_fs *sfs = sv->sv_absvn.vn_fs->fs_data;
	daddr_t diskblock;
	uint32_t fileblock;
	int result;

	/* Allocate missing blocks if and only if we're writing */
	bool doalloc = (uio->uio_rw==UIO_WRITE);

	KASSERT(skipstart + len <= SFS_BLOCKSIZE);

	/* We're using a global static buffer; it had better be locked */
	KASSERT(vfs_biglock_do_i_hold());

	/* Compute the block offset of this block in the file */
	fileblock = uio->uio_offset / SFS_BLOCKSIZE;

	/* Get the disk block number */
	result = sfs_bmap(sv, fileblock, doalloc, &diskblock);
	if (result) {
		return result;
	}

	if (diskblock == 0) {
		/*
		 * There was no block mapped at this point in the file.
		 * Zero the buffer.
		 */
		KASSERT(uio->uio_rw == UIO_READ);
		bzero(iobuf, sizeof(iobuf));
	}
	else {
		/*
		 * Read the block.
		 */
		result = sfs_readblock(sfs, diskblock, iobuf, sizeof(iobuf));
		if (result) {
			return result;
		}
	}

	/*
	 * Now perform the requested operation into/out of the buffer.
	 */
	result = uiomove(iobuf+skipstart, len, uio);
	if (result) {
		return result;
	}

	/*
	 * If it was a write, write back the modified block.
	 */
	if (uio->uio_rw == UIO_WRITE) {
		result = sfs_writeblock(sfs, diskblock, iobuf, sizeof(iobuf));
		if (result) {
			return result;
		}
	}

	return 0;
}

/*
 * Do I/O (either read or write) of a single whole block.
 */
static
int
sfs_blockio(struct sfs_vnode *sv, struct uio *uio)
{
	struct sfs_fs *sfs = sv->sv_absvn.vn_fs->fs_data;
	daddr_t diskblock;
	uint32_t fileblock;
	int result;
	bool doalloc = (uio->uio_rw==UIO_WRITE);
	off_t saveoff;
	off_t diskoff;
	off_t saveres;
	off_t diskres;

	/* Get the block number within the file */
	fileblock = uio->uio_offset / SFS_BLOCKSIZE;

	/* Look up the disk block number */
	result = sfs_bmap(sv, fileblock, doalloc, &diskblock);
	if (result) {
		return result;
	}

	if (diskblock == 0) {
		/*
		 * No block - fill with zeros.
		 *
		 * We must be reading, or sfs_bmap would have
		 * allocated a block for us.
		 */
		KASSERT(uio->uio_rw == UIO_READ);
		return uiomovezeros(SFS_BLOCKSIZE, uio);
	}

	/*
	 * Do the I/O directly to the uio region. Save the uio_offset,
	 * and substitute one that makes sense to the device.
	 */
	saveoff = uio->uio_offset;
	diskoff = diskblock * SFS_BLOCKSIZE;
	uio->uio_offset = diskoff;

	/*
	 * Temporarily set the residue to be one block size.
	 */
	KASSERT(uio->uio_resid >= SFS_BLOCKSIZE);
	saveres = uio->uio_resid;
	diskres = SFS_BLOCKSIZE;
	uio->uio_resid = diskres;

	result = sfs_rwblock(sfs, uio);

	/*
	 * Now, restore the original uio_offset and uio_resid and update
	 * them by the amount of I/O done.
	 */
	uio->uio_offset = (uio->uio_offset - diskoff) + saveoff;
	uio->uio_resid = (uio->uio_resid - diskres) + saveres;

	return result;
}

/*
 * Do I/O of a whole region of data, whether or not it's block-aligned.
 */
int
sfs_io(struct sfs_vnode *sv, struct uio *uio)
{
	uint32_t blkoff;
	uint32_t nblocks, i;
	int result = 0;
	uint32_t origresid, extraresid = 0;

	origresid = uio->uio_resid;

	/*
	 * If reading, check for EOF. If we can read a partial area,
	 * remember how much extra there was in EXTRARESID so we can
	 * add it back to uio_resid at the end.
	 */
	if (uio->uio_rw == UIO_READ) {
		off_t size = sv->sv_i.sfi_size;
		off_t endpos = uio->uio_offset + uio->uio_resid;

		if (uio->uio_offset >= size) {
			/* At or past EOF - just return */
			return 0;
		}

		if (endpos > size) {
			extraresid = endpos - size;
			KASSERT(uio->uio_resid > extraresid);
			uio->uio_resid -= extraresid;
		}
	}

	/*
	 * First, do any leading partial block.
	 */
	blkoff = uio->uio_offset % SFS_BLOCKSIZE;
	if (blkoff != 0) {
		/* Number of bytes at beginning of block to skip */
		uint32_t skip = blkoff;

		/* Number of bytes to read/write after that point */
		uint32_t len = SFS_BLOCKSIZE - blkoff;

		/* ...which might be less than the rest of the block */
		if (len > uio->uio_resid) {
			len = uio->uio_resid;
		}

		/* Call sfs_partialio() to do it. */
		result = sfs_partialio(sv, uio, skip, len);
		if (result) {
			goto out;
		}
	}

	/* If we're done, quit. */
	if (uio->uio_resid==0) {
		goto out;
	}

	/*
	 * Now we should be block-aligned. Do the remaining whole blocks.
	 */
	KASSERT(uio->uio_offset % SFS_BLOCKSIZE == 0);
	nblocks = uio->uio_resid / SFS_BLOCKSIZE;
	for (i=0; i<nblocks; i++) {
		result = sfs_blockio(sv, uio);
		if (result) {
			goto out;
		}
	}

	/*
	 * Now do any remaining partial block at the end.
	 */
	KASSERT(uio->uio_resid < SFS_BLOCKSIZE);

	if (uio->uio_resid > 0) {
		result = sfs_partialio(sv, uio, 0, uio->uio_resid);
		if (result) {
			goto out;
		}
	}

 out:

	/* If writing and we did anything, adjust file length */
	if (uio->uio_resid != origresid &&
	    uio->uio_rw == UIO_WRITE &&
	    uio->uio_offset > (off_t)sv->sv_i.sfi_size) {
		sv->sv_i.sfi_size = uio->uio_offset;
		sv->sv_dirty = true;
	}

	/* Add in any extra amount we couldn't read because of EOF */
	uio->uio_resid += extraresid;

	/* Done */
	return result;
}

////////////////////////////////////////////////////////////
// Metadata I/O

/*
 * This is much the same as sfs_partialio, but intended for use with
 * metadata (e.g. directory entries). It assumes the objects being
 * handled are smaller than whole blocks, do not cross block
 * boundaries, and originate in the kernel.
 *
 * It is separate from sfs_partialio because, although there is no
 * such code in this version of SFS, it is often desirable when doing
 * more advanced things to handle metadata and user data I/O
 * differently.
 */
int
sfs_metaio(struct sfs_vnode *sv, off_t actualpos, void *data, size_t len,
	   enum uio_rw rw)
{
	struct sfs_fs *sfs = sv->sv_absvn.vn_fs->fs_data;
	off_t endpos;
	uint32_t vnblock;
	uint32_t blockoffset;
	daddr_t diskblock;
	bool doalloc;
	int result;

	/*
	 * I/O buffer for metadata ops.
	 *
	 * Note: in real life (and when you've done the fs assignment) you
	 * would get space from the disk buffer cache for this, not use a
	 * static area.
	 */
	static char metaiobuf[SFS_BLOCKSIZE];

	/* We're using a global static buffer; it had better be locked */
	KASSERT(vfs_biglock_do_i_hold());

	/* Figure out which block of the vnode (directory, whatever) this is */
	vnblock = actualpos / SFS_BLOCKSIZE;
	blockoffset = actualpos % SFS_BLOCKSIZE;

	/* Get the disk block number */
	doalloc = (rw == UIO_WRITE);
	result = sfs_bmap(sv, vnblock, doalloc, &diskblock);
	if (result) {
		return result;
	}

	if (diskblock == 0) {
		/* Should only get block 0 back if doalloc is false */
		KASSERT(rw == UIO_READ);

		/* Sparse file, read as zeros. */
		bzero(data, len);
		return 0;
	}

	/* Read the block */
	result = sfs_readblock(sfs, diskblock, metaiobuf, sizeof(metaiobuf));
	if (result) {
		return result;
	}

	if (rw == UIO_READ) {
		/* Copy out the selected region */
		memcpy(data, metaiobuf + blockoffset, len);
	}
	else {
		/* Update the selected region */
		memcpy(metaiobuf + blockoffset, data, len);

		/* Write the block back */
		result = sfs_writeblock(sfs, diskblock,
					metaiobuf, sizeof(metaiobuf));
		if (result) {
			return result;
		}

		/* Update the vnode size if needed */
		endpos = actualpos + len;
		if (endpos > (off_t)sv->sv_i.sfi_size) {
			sv->sv_i.sfi_size = endpos;
			sv->sv_dirty = true;
		}
	}

	/* Done */
	return 0;
}
