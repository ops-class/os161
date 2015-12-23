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

#include "utils.h"
#include "sfs.h"
#include "freemap.h"
#include "inode.h"
#include "main.h"

/*
 * Stuff we remember about inodes.
 * FUTURE: should count the number of blocks allocated to this inode
 */
struct inodeinfo {
	uint32_t ino;
	uint32_t linkcount;	/* files only */
	int visited;		/* dirs only */
	int type;
};

/* Table of inodes found. */
static struct inodeinfo *inodes = NULL;
static unsigned ninodes = 0, maxinodes = 0;

/* Whether the table is sorted and can be looked up with binary search. */
static int inodes_sorted = 0;

////////////////////////////////////////////////////////////
// inode table ops

/*
 * Add an entry to the inode table, realloc'ing it if needed.
 */
static
void
inode_addtable(uint32_t ino, int type)
{
	unsigned newmax;

	assert(ninodes <= maxinodes);
	if (ninodes == maxinodes) {
		newmax = maxinodes ? maxinodes * 2 : 4;
		inodes = dorealloc(inodes, maxinodes * sizeof(inodes[0]),
				   newmax * sizeof(inodes[0]));
		maxinodes = newmax;
	}
	inodes[ninodes].ino = ino;
	inodes[ninodes].linkcount = 0;
	inodes[ninodes].visited = 0;
	inodes[ninodes].type = type;
	ninodes++;
	inodes_sorted = 0;
}

/*
 * Compare function for inodes.
 */
static
int
inode_compare(const void *av, const void *bv)
{
	const struct inodeinfo *a = av;
	const struct inodeinfo *b = bv;

	if (a->ino < b->ino) {
		return -1;
	}
	if (a->ino > b->ino) {
		return 1;
	}
	/*
	 * There should be no duplicates in the table! But C99 makes
	 * no guarantees about whether the implementation of qsort can
	 * ask us to compare an element to itself. Assert that this is
	 * what happened.
	 */
	assert(av == bv);
	return 0;
}

/*
 * After pass1, we sort the inode table for faster access.
 */
void
inode_sorttable(void)
{
	qsort(inodes, ninodes, sizeof(inodes[0]), inode_compare);
	inodes_sorted = 1;
}

/*
 * Find an inode by binary search.
 *
 * This will error out if asked for an inode not in the table; that's
 * not supposed to happen. (This might need to change; if we improve
 * the handling of crosslinked directories as suggested in comments in
 * pass2.c, we'll need to be able to ask if an inode number is valid
 * and names a directory.)
 */
static
struct inodeinfo *
inode_find(uint32_t ino)
{
	unsigned min, max, i;

	assert(inodes_sorted);
	assert(ninodes > 0);

	min = 0;
	max = ninodes;
	while (1) {
		assert(min <= max);
		if (min == max) {
			errx(EXIT_UNRECOV, "FATAL: inode %u wasn't found in my inode table", ino);
		}
		i = min + (max - min)/2;
		if (inodes[i].ino < ino) {
			min = i + 1;
		}
		else if (inodes[i].ino > ino) {
			max = i;
		}
		else {
			assert(inodes[i].ino == ino);
			return &inodes[i];
		}
	}
	/* NOTREACHED */
}

////////////////////////////////////////////////////////////
// inode ops

/*
 * Add an inode; returns 1 if we've already seen it.
 *
 * Uses linear search because we only sort the table for faster access
 * after all inodes have been added. In the FUTURE this could be
 * changed to a better data structure.
 */
int
inode_add(uint32_t ino, int type)
{
	unsigned i;

	for (i=0; i<ninodes; i++) {
		if (inodes[i].ino==ino) {
			assert(inodes[i].linkcount == 0);
			assert(inodes[i].type == type);
			return 1;
		}
	}

	inode_addtable(ino, type);

	return 0;
}

/*
 * Mark an inode (directories only, because that's all the caller
 * does) visited. Returns nonzero if already visited.
 *
 * Note that there is no way to clear the visited flag for now because
 * it's only used once (by pass2).
 */
int
inode_visitdir(uint32_t ino)
{
	struct inodeinfo *inf;

	inf = inode_find(ino);
	assert(inf->type == SFS_TYPE_DIR);
	assert(inf->linkcount == 0);
	if (inf->visited) {
		return 1;
	}
	inf->visited = 1;
	return 0;
}

/*
 * Count a link. Only for regular files because that's what the caller
 * does. (And that, in turn, is because the link count of a directory
 * is a local property.)
 */
void
inode_addlink(uint32_t ino)
{
	struct inodeinfo *inf;

	inf = inode_find(ino);
	assert(inf->type == SFS_TYPE_FILE);
	assert(inf->visited == 0);
	inf->linkcount++;
}

/*
 * Correct link counts. This is effectively pass3. (FUTURE: change the
 * name accordingly.)
 */
void
inode_adjust_filelinks(void)
{
	struct sfs_dinode sfi;
	unsigned i;

	for (i=0; i<ninodes; i++) {
		if (inodes[i].type == SFS_TYPE_DIR) {
			/* directory */
			continue;
		}
		assert(inodes[i].type == SFS_TYPE_FILE);

		/* because we've seen it, there must be at least one link */
		assert(inodes[i].linkcount > 0);

		sfs_readinode(inodes[i].ino, &sfi);
		assert(sfi.sfi_type == SFS_TYPE_FILE);

		if (sfi.sfi_linkcount != inodes[i].linkcount) {
			warnx("File %lu link count %lu should be %lu (fixed)",
			      (unsigned long) inodes[i].ino,
			      (unsigned long) sfi.sfi_linkcount,
			      (unsigned long) inodes[i].linkcount);
			sfi.sfi_linkcount = inodes[i].linkcount;
			setbadness(EXIT_RECOV);
			sfs_writeinode(inodes[i].ino, &sfi);
		}
	}
}

