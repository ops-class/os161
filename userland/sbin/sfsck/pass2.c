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

/*
 * Process a directory. INO is the inode number; PARENTINO is the
 * parent's inode number; PATHSOFAR is the path to this directory.
 *
 * Recursively checks its subdirs.
 *
 * In the FUTURE we might want to improve the handling of crosslinked
 * directories so it picks the parent that the .. entry points to,
 * instead of the first entry we recursively find. Beware of course
 * that the .. entry might not point to anywhere valid at all...
 */
static
int
pass2_dir(uint32_t ino, uint32_t parentino, const char *pathsofar)
{
	struct sfs_dinode sfi;
	struct sfs_direntry *direntries;
	int *sortvector;
	uint32_t dirsize, ndirentries, maxdirentries, subdircount, i;
	int ichanged=0, dchanged=0, dotseen=0, dotdotseen=0;

	if (inode_visitdir(ino)) {
		/* crosslinked dir; tell parent to remove the entry */
		return 1;
	}

	/* Load the inode. */
	sfs_readinode(ino, &sfi);

	/*
	 * Load the directory. If there is any leftover room in the
	 * last block, allocate space for it in case we want to insert
	 * entries.
	 */

	ndirentries = sfi.sfi_size/sizeof(struct sfs_direntry);
	maxdirentries = SFS_ROUNDUP(ndirentries,
				    SFS_BLOCKSIZE/sizeof(struct sfs_direntry));
	dirsize = maxdirentries * sizeof(struct sfs_direntry);
	direntries = domalloc(dirsize);

	sortvector = domalloc(ndirentries * sizeof(int));

	sfs_readdir(&sfi, direntries, ndirentries);
	for (i=ndirentries; i<maxdirentries; i++) {
		direntries[i].sfd_ino = SFS_NOINO;
		bzero(direntries[i].sfd_name, sizeof(direntries[i].sfd_name));
	}

	/*
	 * Sort by name and check for duplicate names.
	 */

	sfsdir_sort(direntries, ndirentries, sortvector);

	/* don't use ndirentries-1 here, in case ndirentries == 0 */
	for (i=0; i+1<ndirentries; i++) {
		struct sfs_direntry *d1 = &direntries[sortvector[i]];
		struct sfs_direntry *d2 = &direntries[sortvector[i+1]];
		assert(d1 != d2);

		if (d1->sfd_ino == SFS_NOINO || d2->sfd_ino == SFS_NOINO) {
			/* sfsdir_sort puts these last */
			continue;
		}

		if (!strcmp(d1->sfd_name, d2->sfd_name)) {
			if (d1->sfd_ino == d2->sfd_ino) {
				setbadness(EXIT_RECOV);
				warnx("Directory %s: Duplicate entries for "
				      "%s (merged)",
				      pathsofar, d1->sfd_name);
				d1->sfd_ino = SFS_NOINO;
				d1->sfd_name[0] = 0;
			}
			else {
				/* XXX: what if FSCK.n.m already exists? */
				snprintf(d1->sfd_name, sizeof(d1->sfd_name),
					 "FSCK.%lu.%lu",
					 (unsigned long) d1->sfd_ino,
					 (unsigned long) uniqueid());
				setbadness(EXIT_RECOV);
				warnx("Directory %s: Duplicate names %s "
				      "(one renamed: %s)",
				      pathsofar, d2->sfd_name, d1->sfd_name);
			}
			dchanged = 1;
		}
	}

	/*
	 * Look for the . and .. entries.
	 */

	for (i=0; i<ndirentries; i++) {
		if (!strcmp(direntries[i].sfd_name, ".")) {
			if (direntries[i].sfd_ino != ino) {
				setbadness(EXIT_RECOV);
				warnx("Directory %s: Incorrect `.' entry "
				      "(fixed)", pathsofar);
				direntries[i].sfd_ino = ino;
				dchanged = 1;
			}
			/* duplicates are checked above -> only one . here */
			assert(dotseen==0);
			dotseen = 1;
		}
		else if (!strcmp(direntries[i].sfd_name, "..")) {
			if (direntries[i].sfd_ino != parentino) {
				setbadness(EXIT_RECOV);
				warnx("Directory %s: Incorrect `..' entry "
				      "(fixed)", pathsofar);
				direntries[i].sfd_ino = parentino;
				dchanged = 1;
			}
			/* duplicates are checked above -> only one .. here */
			assert(dotdotseen==0);
			dotdotseen = 1;
		}
	}

	/*
	 * If no . entry, try to insert one.
	 */

	if (!dotseen) {
		if (sfsdir_tryadd(direntries, ndirentries, ".", ino)==0) {
			setbadness(EXIT_RECOV);
			warnx("Directory %s: No `.' entry (added)",
			      pathsofar);
			dchanged = 1;
		}
		else if (sfsdir_tryadd(direntries, maxdirentries, ".",
				       ino)==0) {
			setbadness(EXIT_RECOV);
			warnx("Directory %s: No `.' entry (added)",
			      pathsofar);
			ndirentries++;
			dchanged = 1;
			sfi.sfi_size += sizeof(struct sfs_direntry);
			ichanged = 1;
		}
		else {
			setbadness(EXIT_UNRECOV);
			warnx("Directory %s: No `.' entry (NOT FIXED)",
			      pathsofar);
		}
	}

	/*
	 * If no .. entry, try to insert one.
	 */

	if (!dotdotseen) {
		if (sfsdir_tryadd(direntries, ndirentries, "..",
				  parentino)==0) {
			setbadness(EXIT_RECOV);
			warnx("Directory %s: No `..' entry (added)",
			      pathsofar);
			dchanged = 1;
		}
		else if (sfsdir_tryadd(direntries, maxdirentries, "..",
				    parentino)==0) {
			setbadness(EXIT_RECOV);
			warnx("Directory %s: No `..' entry (added)",
			      pathsofar);
			ndirentries++;
			dchanged = 1;
			sfi.sfi_size += sizeof(struct sfs_direntry);
			ichanged = 1;
		}
		else {
			setbadness(EXIT_UNRECOV);
			warnx("Directory %s: No `..' entry (NOT FIXED)",
			      pathsofar);
		}
	}

	/*
	 * Now load each inode in the directory.
	 *
	 * For regular files, count the number of links we see; for
	 * directories, recurse. Count the number of subdirs seen
	 * so we can correct our own link count if necessary.
	 */

	subdircount=0;
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

			sfs_readinode(direntries[i].sfd_ino, &subsfi);
			snprintf(path, sizeof(path), "%s/%s",
				 pathsofar, direntries[i].sfd_name);

			switch (subsfi.sfi_type) {
			    case SFS_TYPE_FILE:
				inode_addlink(direntries[i].sfd_ino);
				break;
			    case SFS_TYPE_DIR:
				if (pass2_dir(direntries[i].sfd_ino,
					      ino,
					      path)) {
					setbadness(EXIT_RECOV);
					warnx("Directory %s: Crosslink to "
					      "other directory (removed)",
					      path);
					direntries[i].sfd_ino = SFS_NOINO;
					direntries[i].sfd_name[0] = 0;
					dchanged = 1;
				}
				else {
					subdircount++;
				}
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

	/*
	 * Fix up the link count if needed.
	 */

	if (sfi.sfi_linkcount != subdircount+2) {
		setbadness(EXIT_RECOV);
		warnx("Directory %s: Link count %lu should be %lu (fixed)",
		      pathsofar, (unsigned long) sfi.sfi_linkcount,
		      (unsigned long) subdircount+2);
		sfi.sfi_linkcount = subdircount+2;
		ichanged = 1;
	}

	/*
	 * Write back anything that changed, clean up, and return.
	 */

	if (dchanged) {
		sfs_writedir(&sfi, direntries, ndirentries);
	}

	if (ichanged) {
		sfs_writeinode(ino, &sfi);
	}

	free(direntries);
	free(sortvector);

	return 0;
}

void
pass2(void)
{
	char path[SFS_VOLNAME_SIZE + 2];

	snprintf(path, sizeof(path), "%s:", sb_volname());
	pass2_dir(SFS_ROOTDIR_INO, SFS_ROOTDIR_INO, path);
}
