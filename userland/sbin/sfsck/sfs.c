/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2009, 2013
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <err.h>

#include "compat.h"
#include <kern/sfs.h>

#include "disk.h"
#include "utils.h"
#include "ibmacros.h"
#include "sfs.h"
#include "main.h"

////////////////////////////////////////////////////////////
// global setup

void
sfs_setup(void)
{
	assert(sizeof(struct sfs_superblock)==SFS_BLOCKSIZE);
	assert(sizeof(struct sfs_dinode)==SFS_BLOCKSIZE);
	assert(SFS_BLOCKSIZE % sizeof(struct sfs_direntry) == 0);
}

////////////////////////////////////////////////////////////
// byte-swap functions

static
void
swapsb(struct sfs_superblock *sb)
{
	sb->sb_magic = SWAP32(sb->sb_magic);
	sb->sb_nblocks = SWAP32(sb->sb_nblocks);
}

static
void
swapbits(uint8_t *bits)
{
	/* nothing to do */
	(void)bits;
}

static
void
swapinode(struct sfs_dinode *sfi)
{
	int i;

	sfi->sfi_size = SWAP32(sfi->sfi_size);
	sfi->sfi_type = SWAP16(sfi->sfi_type);
	sfi->sfi_linkcount = SWAP16(sfi->sfi_linkcount);

	for (i=0; i<NUM_D; i++) {
		SET_D(sfi, i) = SWAP32(GET_D(sfi, i));
	}

	for (i=0; i<NUM_I; i++) {
		SET_I(sfi, i) = SWAP32(GET_I(sfi, i));
	}

	for (i=0; i<NUM_II; i++) {
		SET_II(sfi, i) = SWAP32(GET_II(sfi, i));
	}

	for (i=0; i<NUM_III; i++) {
		SET_III(sfi, i) = SWAP32(GET_III(sfi, i));
	}
}

static
void
swapdir(struct sfs_direntry *sfd)
{
	sfd->sfd_ino = SWAP32(sfd->sfd_ino);
}

static
void
swapindir(uint32_t *entries)
{
	int i;
	for (i=0; i<SFS_DBPERIDB; i++) {
		entries[i] = SWAP32(entries[i]);
	}
}

////////////////////////////////////////////////////////////
// bmap()

/*
 * Indirect block bmap: in indirect block IBLOCK, read the entry at
 * block OFFSET from the first file block mapped by this indirect
 * block.
 *
 * ENTRYSIZE is how many blocks each entry describes; for a
 * singly-indirect block this is 1. For a multiply-indirect block,
 * it is more than 1; in this case recurse.
 */
static
uint32_t
ibmap(uint32_t iblock, uint32_t offset, uint32_t entrysize)
{
	uint32_t entries[SFS_DBPERIDB];

	if (iblock == 0) {
		return 0;
	}

	diskread(entries, iblock);
	swapindir(entries);

	if (entrysize > 1) {
		uint32_t index = offset / entrysize;
		offset %= entrysize;
		return ibmap(entries[index], offset, entrysize/SFS_DBPERIDB);
	}
	else {
		assert(offset < SFS_DBPERIDB);
		return entries[offset];
	}
}

/*
 * bmap() for SFS.
 *
 * Given an inode and a file block, returns a disk block.
 */
static
uint32_t
bmap(const struct sfs_dinode *sfi, uint32_t fileblock)
{
	uint32_t iblock, offset;

	if (fileblock < INOMAX_D) {
		return GET_D(sfi, fileblock);
	}
	else if (fileblock < INOMAX_I) {
		iblock = (fileblock - INOMAX_D) / RANGE_I;
		offset = (fileblock - INOMAX_D) % RANGE_I;
		return ibmap(GET_I(sfi, iblock), offset, RANGE_D);
	}
	else if (fileblock < INOMAX_II) {
		iblock = (fileblock - INOMAX_I) / RANGE_II;
		offset = (fileblock - INOMAX_I) % RANGE_II;
		return ibmap(GET_II(sfi, iblock), offset, RANGE_I);
	}
	else if (fileblock < INOMAX_III) {
		iblock = (fileblock - INOMAX_II) / RANGE_III;
		offset = (fileblock - INOMAX_II) % RANGE_III;
		return ibmap(GET_III(sfi, iblock), offset, RANGE_II);
	}
	return 0;
}

////////////////////////////////////////////////////////////
// superblock, free block bitmap, and inode I/O

/*
 *  superblock - blocknum is a disk block number.
 */

void
sfs_readsb(uint32_t blocknum, struct sfs_superblock *sb)
{
	diskread(sb, blocknum);
	swapsb(sb);
}

void
sfs_writesb(uint32_t blocknum, struct sfs_superblock *sb)
{
	swapsb(sb);
	diskwrite(sb, blocknum);
	swapsb(sb);
}

/*
 * freemap blocks - whichblock is a block number within the free block
 * bitmap.
 */

void
sfs_readfreemapblock(uint32_t whichblock, uint8_t *bits)
{
	diskread(bits, SFS_FREEMAP_START + whichblock);
	swapbits(bits);
}

void
sfs_writefreemapblock(uint32_t whichblock, uint8_t *bits)
{
	swapbits(bits);
	diskwrite(bits, SFS_FREEMAP_START + whichblock);
	swapbits(bits);
}

/*
 *  inodes - ino is an inode number, which is a disk block number.
 */

void
sfs_readinode(uint32_t ino, struct sfs_dinode *sfi)
{
	diskread(sfi, ino);
	swapinode(sfi);
}

void
sfs_writeinode(uint32_t ino, struct sfs_dinode *sfi)
{
	swapinode(sfi);
	diskwrite(sfi, ino);
	swapinode(sfi);
}

/*
 *  indirect blocks - blocknum is a disk block number.
 */

void
sfs_readindirect(uint32_t blocknum, uint32_t *entries)
{
	diskread(entries, blocknum);
	swapindir(entries);
}

void
sfs_writeindirect(uint32_t blocknum, uint32_t *entries)
{
	swapindir(entries);
	diskwrite(entries, blocknum);
	swapindir(entries);
}

////////////////////////////////////////////////////////////
// directory I/O

/*
 * Read the directory block at DISKBLOCK into D.
 */
static
void
sfs_readdirblock(struct sfs_direntry *d, uint32_t diskblock)
{
	const unsigned atonce = SFS_BLOCKSIZE/sizeof(struct sfs_direntry);
	unsigned j;

	if (diskblock != 0) {
		diskread(d, diskblock);
		for (j=0; j<atonce; j++) {
			swapdir(&d[j]);
		}
	}
	else {
		warnx("Warning: sparse directory found");
		bzero(d, SFS_BLOCKSIZE);
	}
}

/*
 * Read in a directory, from the inode SFI, into D, which is a buffer
 * with ND slots. The caller is assumed to have figured out the right
 * number of slots.
 */
void
sfs_readdir(struct sfs_dinode *sfi, struct sfs_direntry *d, unsigned nd)
{
	const unsigned atonce = SFS_BLOCKSIZE/sizeof(struct sfs_direntry);
	unsigned nblocks = SFS_ROUNDUP(nd, atonce) / atonce;
	unsigned i, j;
	unsigned left, thismany;
	struct sfs_direntry buffer[atonce];
	uint32_t diskblock;

	left = nd;
	for (i=0; i<nblocks; i++) {
		diskblock = bmap(sfi, i);
		if (left < atonce) {
			thismany = left;
			sfs_readdirblock(buffer, diskblock);
			for (j=0; j<thismany; j++) {
				d[i*atonce + j] = buffer[j];
			}
		}
		else {
			thismany = atonce;
			sfs_readdirblock(d + i*atonce, diskblock);
		}
		left -= thismany;
	}
	assert(left == 0);
}

/*
 * Write the directory block D to DISKBLOCK.
 */
static
void
sfs_writedirblock(struct sfs_direntry *d, uint32_t diskblock)
{
	const unsigned atonce = SFS_BLOCKSIZE/sizeof(struct sfs_direntry);
	unsigned j, bad;

	if (diskblock != 0) {
		for (j=0; j<atonce; j++) {
			swapdir(&d[j]);
		}
		diskwrite(d, diskblock);
	}
	else {
		for (j=bad=0; j<atonce; j++) {
			if (d[j].sfd_ino != SFS_NOINO ||
			    d[j].sfd_name[0] != 0) {
				bad = 1;
			}
		}
		if (bad) {
			warnx("Cannot write to missing block in "
			      "sparse directory (ERROR)");
			setbadness(EXIT_UNRECOV);
		}
	}
}

/*
 * Write out a directory, from the inode SFI, using D, which is a
 * buffer with ND slots. The caller is assumed to have set the inode
 * size accordingly.
 */
void
sfs_writedir(const struct sfs_dinode *sfi, struct sfs_direntry *d, unsigned nd)
{
	const unsigned atonce = SFS_BLOCKSIZE/sizeof(struct sfs_direntry);
	unsigned nblocks = SFS_ROUNDUP(nd, atonce) / atonce;
	unsigned i, j;
	unsigned left, thismany;
	struct sfs_direntry buffer[atonce];
	uint32_t diskblock;

	left = nd;
	for (i=0; i<nblocks; i++) {
		diskblock = bmap(sfi, i);
		if (left < atonce) {
			thismany = left;
			for (j=0; j<thismany; j++) {
				buffer[j] = d[i*atonce + j];
			}
			for (; j<atonce; j++) {
				memset(&buffer[j], 0, sizeof(buffer[j]));
			}
			sfs_writedirblock(buffer, diskblock);
		}
		else {
			thismany = atonce;
			sfs_writedirblock(d + i*atonce, diskblock);
		}
		left -= thismany;
	}
	assert(left == 0);
}

////////////////////////////////////////////////////////////
// directory utilities

/* this exists because qsort() doesn't pass a context pointer through */
static struct sfs_direntry *global_sortdirs;

/*
 * Compare function for the permutation vector produced by
 * sfsdir_sort().
 */
static
int
dirsortfunc(const void *aa, const void *bb)
{
	const int *a = (const int *)aa;
	const int *b = (const int *)bb;
	const struct sfs_direntry *ad = &global_sortdirs[*a];
	const struct sfs_direntry *bd = &global_sortdirs[*b];

	/* Sort unallocated entries last */
	if (ad->sfd_ino == SFS_NOINO && bd->sfd_ino == SFS_NOINO) {
		return 0;
	}
	if (ad->sfd_ino == SFS_NOINO) {
		return 1;
	}
	if (bd->sfd_ino == SFS_NOINO) {
		return -1;
	}

	return strcmp(ad->sfd_name, bd->sfd_name);
}

/*
 * Sort the directory contents in D (with ND entries) by producing a
 * permutation vector into VECTOR, which should be allocated to hold
 * ND ints.
 */
void
sfsdir_sort(struct sfs_direntry *d, unsigned nd, int *vector)
{
	unsigned i;

	for (i=0; i<nd; i++) {
		vector[i] = i;
	}

	global_sortdirs = d;
	qsort(vector, nd, sizeof(int), dirsortfunc);
}

/*
 * Try to add an entry NAME/INO to D (which has ND entries) by
 * finding an empty slot. Cannot allocate new space.
 *
 * Returns 0 on success and nonzero on failure.
 */
int
sfsdir_tryadd(struct sfs_direntry *d, int nd, const char *name, uint32_t ino)
{
	int i;
	for (i=0; i<nd; i++) {
		if (d[i].sfd_ino==SFS_NOINO) {
			d[i].sfd_ino = ino;
			assert(strlen(name) < sizeof(d[i].sfd_name));
			strcpy(d[i].sfd_name, name);
			return 0;
		}
	}
	return -1;
}
