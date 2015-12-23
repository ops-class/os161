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
 * Directory I/O
 */
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <vfs.h>
#include <sfs.h>
#include "sfsprivate.h"

/*
 * Read the directory entry out of slot SLOT of a directory vnode.
 * The "slot" is the index of the directory entry, starting at 0.
 */
static
int
sfs_readdir(struct sfs_vnode *sv, int slot, struct sfs_direntry *sd)
{
	off_t actualpos;

	/* Compute the actual position in the directory to read. */
	actualpos = slot * sizeof(struct sfs_direntry);

	return sfs_metaio(sv, actualpos, sd, sizeof(*sd), UIO_READ);
}

/*
 * Write (overwrite) the directory entry in slot SLOT of a directory
 * vnode.
 */
static
int
sfs_writedir(struct sfs_vnode *sv, int slot, struct sfs_direntry *sd)
{
	off_t actualpos;

	/* Compute the actual position in the directory. */
	KASSERT(slot>=0);
	actualpos = slot * sizeof(struct sfs_direntry);

	return sfs_metaio(sv, actualpos, sd, sizeof(*sd), UIO_WRITE);
}

/*
 * Compute the number of entries in a directory.
 * This actually computes the number of existing slots, and does not
 * account for empty slots.
 */
static
int
sfs_dir_nentries(struct sfs_vnode *sv)
{
	struct sfs_fs *sfs = sv->sv_absvn.vn_fs->fs_data;
	off_t size;

	KASSERT(sv->sv_i.sfi_type == SFS_TYPE_DIR);

	size = sv->sv_i.sfi_size;
	if (size % sizeof(struct sfs_direntry) != 0) {
		panic("sfs: %s: directory %u: Invalid size %llu\n",
		      sfs->sfs_sb.sb_volname, sv->sv_ino, size);
	}

	return size / sizeof(struct sfs_direntry);
}

/*
 * Search a directory for a particular filename in a directory, and
 * return its inode number, its slot, and/or the slot number of an
 * empty directory slot if one is found.
 */
int
sfs_dir_findname(struct sfs_vnode *sv, const char *name,
		uint32_t *ino, int *slot, int *emptyslot)
{
	struct sfs_direntry tsd;
	int found, nentries, i, result;

	nentries = sfs_dir_nentries(sv);

	/* For each slot... */
	found = 0;
	for (i=0; i<nentries; i++) {

		/* Read the entry from that slot */
		result = sfs_readdir(sv, i, &tsd);
		if (result) {
			return result;
		}
		if (tsd.sfd_ino == SFS_NOINO) {
			/* Free slot - report it back if one was requested */
			if (emptyslot != NULL) {
				*emptyslot = i;
			}
		}
		else {
			/* Ensure null termination, just in case */
			tsd.sfd_name[sizeof(tsd.sfd_name)-1] = 0;
			if (!strcmp(tsd.sfd_name, name)) {

				/* Each name may legally appear only once... */
				KASSERT(found==0);

				found = 1;
				if (slot != NULL) {
					*slot = i;
				}
				if (ino != NULL) {
					*ino = tsd.sfd_ino;
				}
			}
		}
	}

	return found ? 0 : ENOENT;
}

/*
 * Create a link in a directory to the specified inode by number, with
 * the specified name, and optionally hand back the slot.
 */
int
sfs_dir_link(struct sfs_vnode *sv, const char *name, uint32_t ino, int *slot)
{
	int emptyslot = -1;
	int result;
	struct sfs_direntry sd;

	/* Look up the name. We want to make sure it *doesn't* exist. */
	result = sfs_dir_findname(sv, name, NULL, NULL, &emptyslot);
	if (result!=0 && result!=ENOENT) {
		return result;
	}
	if (result==0) {
		return EEXIST;
	}

	if (strlen(name)+1 > sizeof(sd.sfd_name)) {
		return ENAMETOOLONG;
	}

	/* If we didn't get an empty slot, add the entry at the end. */
	if (emptyslot < 0) {
		emptyslot = sfs_dir_nentries(sv);
	}

	/* Set up the entry. */
	bzero(&sd, sizeof(sd));
	sd.sfd_ino = ino;
	strcpy(sd.sfd_name, name);

	/* Hand back the slot, if so requested. */
	if (slot) {
		*slot = emptyslot;
	}

	/* Write the entry. */
	return sfs_writedir(sv, emptyslot, &sd);
}

/*
 * Unlink a name in a directory, by slot number.
 */
int
sfs_dir_unlink(struct sfs_vnode *sv, int slot)
{
	struct sfs_direntry sd;

	/* Initialize a suitable directory entry... */
	bzero(&sd, sizeof(sd));
	sd.sfd_ino = SFS_NOINO;

	/* ... and write it */
	return sfs_writedir(sv, slot, &sd);
}

/*
 * Look for a name in a directory and hand back a vnode for the
 * file, if there is one.
 */
int
sfs_lookonce(struct sfs_vnode *sv, const char *name,
		struct sfs_vnode **ret,
		int *slot)
{
	struct sfs_fs *sfs = sv->sv_absvn.vn_fs->fs_data;
	uint32_t ino;
	int result;

	result = sfs_dir_findname(sv, name, &ino, slot, NULL);
	if (result) {
		return result;
	}

	result = sfs_loadvnode(sfs, ino, SFS_TYPE_INVAL, ret);
	if (result) {
		return result;
	}

	if ((*ret)->sv_i.sfi_linkcount == 0) {
		panic("sfs: %s: name %s (inode %u) in dir %u has "
		      "linkcount 0\n", sfs->sfs_sb.sb_volname,
		      name, (*ret)->sv_ino, sv->sv_ino);
	}

	return 0;
}

