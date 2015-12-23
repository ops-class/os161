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
#include <stdio.h>
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
#include "sb.h"
#include "freemap.h"
#include "inode.h"
#include "passes.h"
#include "main.h"

static unsigned long count_dirs=0, count_files=0;

/*
 * State for checking indirect blocks.
 */
struct ibstate {
	uint32_t ino;		/* inode we're doing (constant) */
	uint32_t curfileblock;	/* current block offset in the file */
	uint32_t fileblocks;	/* file size in blocks (constant) */
	uint32_t volblocks;	/* volume size in blocks (constant) */
	unsigned pasteofcount;	/* number of blocks found past eof */
	blockusage_t usagetype;	/* how to call freemap_blockinuse() */
};

/*
 * Traverse an indirect block, recording blocks that are in use,
 * dropping any entries that are past EOF, and clearing any entries
 * that point outside the volume.
 *
 * XXX: this should be extended to be able to recover from crosslinked
 * blocks. Currently it just complains in freemap.c and sets
 * EXIT_UNRECOV.
 *
 * The traversal is recursive; the state is maintained in IBS (as
 * described above). IENTRY is a pointer to the entry in the parent
 * indirect block (or the inode) that names the block we're currently
 * scanning. IECHANGEDP should be set to 1 if *IENTRY is changed.
 * INDIRECTION is the indirection level of this block (1, 2, or 3).
 */
static
void
check_indirect_block(struct ibstate *ibs, uint32_t *ientry, int *iechangedp,
		     int indirection)
{
	uint32_t entries[SFS_DBPERIDB];
	uint32_t i, ct;
	uint32_t coveredblocks;
	int localchanged = 0;
	int j;

	if (*ientry > 0 && *ientry < ibs->volblocks) {
		sfs_readindirect(*ientry, entries);
		freemap_blockinuse(*ientry, B_IBLOCK, ibs->ino);
	}
	else {
		if (*ientry >= ibs->volblocks) {
			setbadness(EXIT_RECOV);
			warnx("Inode %lu: indirect block pointer (level %d) "
			      "for block %lu outside of volume: %lu "
			      "(cleared)\n",
			      (unsigned long)ibs->ino, indirection,
			      (unsigned long)ibs->curfileblock,
			      (unsigned long)*ientry);
			*ientry = 0;
			*iechangedp = 1;
		}
		coveredblocks = 1;
		for (j=0; j<indirection; j++) {
			coveredblocks *= SFS_DBPERIDB;
		}
		ibs->curfileblock += coveredblocks;
		return;
	}

	if (indirection > 1) {
		for (i=0; i<SFS_DBPERIDB; i++) {
			check_indirect_block(ibs, &entries[i], &localchanged,
					     indirection-1);
		}
	}
	else {
		assert(indirection==1);

		for (i=0; i<SFS_DBPERIDB; i++) {
			if (entries[i] >= ibs->volblocks) {
				setbadness(EXIT_RECOV);
				warnx("Inode %lu: direct block pointer for "
				      "block %lu outside of volume: %lu "
				      "(cleared)\n",
				      (unsigned long)ibs->ino,
				      (unsigned long)ibs->curfileblock,
				      (unsigned long)entries[i]);
				entries[i] = 0;
				localchanged = 1;
			}
			else if (entries[i] != 0) {
				if (ibs->curfileblock < ibs->fileblocks) {
					freemap_blockinuse(entries[i],
							  ibs->usagetype,
							  ibs->ino);
				}
				else {
					setbadness(EXIT_RECOV);
					ibs->pasteofcount++;
					freemap_blockfree(entries[i]);
					entries[i] = 0;
					localchanged = 1;
				}
			}
			ibs->curfileblock++;
		}
	}

	ct=0;
	for (i=ct=0; i<SFS_DBPERIDB; i++) {
		if (entries[i]!=0) ct++;
	}
	if (ct==0) {
		if (*ientry != 0) {
			setbadness(EXIT_RECOV);
			/* this is not necessarily correct */
			/*ibs->pasteofcount++;*/
			*iechangedp = 1;
			freemap_blockfree(*ientry);
			*ientry = 0;
		}
	}
	else {
		assert(*ientry != 0);
		if (localchanged) {
			sfs_writeindirect(*ientry, entries);
		}
	}
}

/*
 * Check the blocks belonging to inode INO, whose inode has already
 * been loaded into SFI. ISDIR is a shortcut telling us if the inode
 * is a directory.
 *
 * Returns nonzero if SFI has been modified and needs to be written
 * back.
 */
static
int
check_inode_blocks(uint32_t ino, struct sfs_dinode *sfi, int isdir)
{
	struct ibstate ibs;
	uint32_t size, datablock;
	int changed;
	int i;

	size = SFS_ROUNDUP(sfi->sfi_size, SFS_BLOCKSIZE);

	ibs.ino = ino;
	/*ibs.curfileblock = 0;*/
	ibs.fileblocks = size/SFS_BLOCKSIZE;
	ibs.volblocks = sb_totalblocks();
	ibs.pasteofcount = 0;
	ibs.usagetype = isdir ? B_DIRDATA : B_DATA;

	changed = 0;

	for (ibs.curfileblock=0; ibs.curfileblock<NUM_D; ibs.curfileblock++) {
		datablock = GET_D(sfi, ibs.curfileblock);
		if (datablock >= ibs.volblocks) {
			setbadness(EXIT_RECOV);
			warnx("Inode %lu: direct block pointer for "
			      "block %lu outside of volume: %lu "
			      "(cleared)\n",
			      (unsigned long)ibs.ino,
			      (unsigned long)ibs.curfileblock,
			      (unsigned long)datablock);
			SET_D(sfi, ibs.curfileblock) = 0;
			changed = 1;
		}
		else if (datablock > 0) {
			if (ibs.curfileblock < ibs.fileblocks) {
				freemap_blockinuse(datablock, ibs.usagetype,
						   ibs.ino);
			}
			else {
				setbadness(EXIT_RECOV);
				ibs.pasteofcount++;
				changed = 1;
				freemap_blockfree(datablock);
				SET_D(sfi, ibs.curfileblock) = 0;
			}
		}
	}

	for (i=0; i<NUM_I; i++) {
		check_indirect_block(&ibs, &SET_I(sfi, i), &changed, 1);
	}
	for (i=0; i<NUM_II; i++) {
		check_indirect_block(&ibs, &SET_II(sfi, i), &changed, 2);
	}
	for (i=0; i<NUM_III; i++) {
		check_indirect_block(&ibs, &SET_III(sfi, i), &changed, 3);
	}

	if (ibs.pasteofcount > 0) {
		warnx("Inode %lu: %u blocks after EOF (freed)",
		     (unsigned long) ibs.ino, ibs.pasteofcount);
		setbadness(EXIT_RECOV);
	}

	return changed;
}

/*
 * Do the pass1 inode-level checks on inode INO, which has already
 * been loaded into SFI. Note that sfi_type has already been
 * validated.
 *
 * Returns nonzero if SFI has been modified and needs to be written
 * back.
 */
static
int
pass1_inode(uint32_t ino, struct sfs_dinode *sfi, int alreadychanged)
{
	int changed = alreadychanged;
	int isdir = sfi->sfi_type == SFS_TYPE_DIR;

	if (inode_add(ino, sfi->sfi_type)) {
		/* Already been here. */
		assert(changed == 0);
		return 1;
	}

	freemap_blockinuse(ino, B_INODE, ino);

	if (checkzeroed(sfi->sfi_waste, sizeof(sfi->sfi_waste))) {
		warnx("Inode %lu: sfi_waste section not zeroed (fixed)",
		      (unsigned long) ino);
		setbadness(EXIT_RECOV);
		changed = 1;
	}

	if (check_inode_blocks(ino, sfi, isdir)) {
		changed = 1;
	}

	if (changed) {
		sfs_writeinode(ino, sfi);
	}
	return 0;
}

/*
 * Check the directory entry in SFD. INDEX is its offset, and PATH is
 * its name; these are used for printing messages.
 */
static
int
pass1_direntry(const char *path, uint32_t index, struct sfs_direntry *sfd)
{
	int dchanged = 0;
	uint32_t nblocks;

	nblocks = sb_totalblocks();

	if (sfd->sfd_ino == SFS_NOINO) {
		if (sfd->sfd_name[0] != 0) {
			setbadness(EXIT_RECOV);
			warnx("Directory %s entry %lu has name but no file",
			      path, (unsigned long) index);
			sfd->sfd_name[0] = 0;
			dchanged = 1;
		}
	}
	else if (sfd->sfd_ino >= nblocks) {
		setbadness(EXIT_RECOV);
		warnx("Directory %s entry %lu has out of range "
		      "inode (cleared)",
		      path, (unsigned long) index);
		sfd->sfd_ino = SFS_NOINO;
		sfd->sfd_name[0] = 0;
		dchanged = 1;
	}
	else {
		if (sfd->sfd_name[0] == 0) {
			/* XXX: what happens if FSCK.n.m already exists? */
			snprintf(sfd->sfd_name, sizeof(sfd->sfd_name),
				 "FSCK.%lu.%lu",
				 (unsigned long) sfd->sfd_ino,
				 (unsigned long) uniqueid());
			setbadness(EXIT_RECOV);
			warnx("Directory %s entry %lu has file but "
			      "no name (fixed: %s)",
			      path, (unsigned long) index,
			      sfd->sfd_name);
			dchanged = 1;
		}
		if (checknullstring(sfd->sfd_name, sizeof(sfd->sfd_name))) {
			setbadness(EXIT_RECOV);
			warnx("Directory %s entry %lu not "
			      "null-terminated (fixed)",
			      path, (unsigned long) index);
			dchanged = 1;
		}
		if (checkbadstring(sfd->sfd_name)) {
			setbadness(EXIT_RECOV);
			warnx("Directory %s entry %lu contains invalid "
			      "characters (fixed)",
			      path, (unsigned long) index);
			dchanged = 1;
		}
	}
	return dchanged;
}

/*
 * Check a directory. INO is the inode number; PATHSOFAR is the path
 * to this directory. This traverses the volume directory tree
 * recursively.
 */
static
void
pass1_dir(uint32_t ino, const char *pathsofar)
{
	struct sfs_dinode sfi;
	struct sfs_direntry *direntries;
	uint32_t ndirentries, i;
	int ichanged=0, dchanged=0;

	sfs_readinode(ino, &sfi);

	if (sfi.sfi_size % sizeof(struct sfs_direntry) != 0) {
		setbadness(EXIT_RECOV);
		warnx("Directory %s has illegal size %lu (fixed)",
		      pathsofar, (unsigned long) sfi.sfi_size);
		sfi.sfi_size = SFS_ROUNDUP(sfi.sfi_size,
					   sizeof(struct sfs_direntry));
		ichanged = 1;
	}
	count_dirs++;

	if (pass1_inode(ino, &sfi, ichanged)) {
		/* been here before; crosslinked dir, sort it out in pass 2 */
		return;
	}

	ndirentries = sfi.sfi_size/sizeof(struct sfs_direntry);
	direntries = domalloc(sfi.sfi_size);

	sfs_readdir(&sfi, direntries, ndirentries);

	for (i=0; i<ndirentries; i++) {
		if (pass1_direntry(pathsofar, i, &direntries[i])) {
			dchanged = 1;
		}
	}

	for (i=0; i<ndirentries; i++) {
		if (direntries[i].sfd_ino == SFS_NOINO) {
			/* nothing */
		}
		else if (!strcmp(direntries[i].sfd_name, ".")) {
			/* nothing */
		}
		else if (!strcmp(direntries[i].sfd_name, "..")) {
			/* nothing */
		}
		else {
			char path[strlen(pathsofar)+SFS_NAMELEN+1];
			struct sfs_dinode subsfi;
			uint32_t subino;

			subino = direntries[i].sfd_ino;
			sfs_readinode(subino, &subsfi);
			snprintf(path, sizeof(path), "%s/%s",
				 pathsofar, direntries[i].sfd_name);

			switch (subsfi.sfi_type) {
			    case SFS_TYPE_FILE:
				if (pass1_inode(subino, &subsfi, 0)) {
					/* been here before */
					break;
				}
				count_files++;
				break;
			    case SFS_TYPE_DIR:
				pass1_dir(subino, path);
				break;
			    default:
				setbadness(EXIT_RECOV);
				warnx("Object %s: Invalid inode type %u "
				      "(removed)", path, subsfi.sfi_type);
				direntries[i].sfd_ino = SFS_NOINO;
				direntries[i].sfd_name[0] = 0;
				dchanged = 1;
				break;
			}
		}
	}

	if (dchanged) {
		sfs_writedir(&sfi, direntries, ndirentries);
	}

	free(direntries);
}

/*
 * Check the root directory, and implicitly everything under it.
 */
static
void
pass1_rootdir(void)
{
	struct sfs_dinode sfi;
	char path[SFS_VOLNAME_SIZE + 2];

	sfs_readinode(SFS_ROOTDIR_INO, &sfi);

	switch (sfi.sfi_type) {
	    case SFS_TYPE_DIR:
		break;
	    case SFS_TYPE_FILE:
		warnx("Root directory inode is a regular file (fixed)");
		goto fix;
	    default:
		warnx("Root directory inode has invalid type %lu (fixed)",
		      (unsigned long) sfi.sfi_type);
	    fix:
		setbadness(EXIT_RECOV);
		sfi.sfi_type = SFS_TYPE_DIR;
		sfs_writeinode(SFS_ROOTDIR_INO, &sfi);
		break;
	}

	snprintf(path, sizeof(path), "%s:", sb_volname());
	pass1_dir(SFS_ROOTDIR_INO, path);
}

////////////////////////////////////////////////////////////
// public interface

void
pass1(void)
{
	pass1_rootdir();
}

unsigned long
pass1_founddirs(void)
{
	return count_dirs;
}

unsigned long
pass1_foundfiles(void)
{
	return count_files;
}
