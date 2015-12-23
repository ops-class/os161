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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <err.h>

#if 0 /* XXX: should add this to libc */
#include <stdbool.h>
#else
typedef unsigned bool;
#define false 0
#define true 1
#endif

#include "ops.h"
#include "workloads.h"

////////////////////////////////////////////////////////////
// support code

static
unsigned long
getnum(const char *str)
{
#if 0 /* sigh */
	unsigned long val;
	char *x;

	errno = 0;
	val = strtoul(str, &x, 0);
	if (errno) {
		err(1, "Invalid number %s", str);
	}
	if (strlen(x) > 0) {
		errx(1, "Invalid junk at end of number %s", str);
	}
	return val;
#else
	return atoi(str);
#endif
}

////////////////////////////////////////////////////////////
// standard sizes

enum sizes {
	SIZE_ONE,
	SIZE_SMALL,
	SIZE_MEDIUM,
	SIZE_LARGE,
	SIZE_LARGEPLUS,
};

/*
 * This only returns the sizes that can be selected from the command
 * line.
 */
static
enum sizes
strtosize(const char *word)
{
	if (!strcmp(word, "small")) {
		return SIZE_SMALL;
	}
	if (!strcmp(word, "medium")) {
		return SIZE_MEDIUM;
	}
	if (!strcmp(word, "large")) {
		return SIZE_LARGE;
	}
	errx(1, "Invalid size %s (try small, medium, or large)", word);
	/* XXX errx should be declared noreturn */
	return SIZE_SMALL;
}

static
enum sizes
randsize(void)
{
	switch (random() % 7) {
	    case 0:
		return SIZE_ONE;
	    case 1:
	    case 2:
	    case 3:
		return SIZE_SMALL;
	    case 4:
	    case 5:
		return SIZE_MEDIUM;
	    case 6:
#if 0 /* too annoying */
		return SIZE_LARGE;
#else
		return SIZE_MEDIUM;
#endif
	}
	return SIZE_ONE;
}

static
enum sizes
nextsmallersize(enum sizes sz)
{
	switch (sz) {
	    case SIZE_ONE:
		break;
	    case SIZE_SMALL:
		return SIZE_ONE;
	    case SIZE_MEDIUM:
		return SIZE_SMALL;
	    case SIZE_LARGE:
		return SIZE_MEDIUM;
	    case SIZE_LARGEPLUS:
		return SIZE_LARGE;
	}
	assert(0);
	return SIZE_ONE;
}

static
enum sizes
nextlargersize(enum sizes sz)
{
	switch (sz) {
	    case SIZE_ONE:
		return SIZE_SMALL;
	    case SIZE_SMALL:
		return SIZE_MEDIUM;
	    case SIZE_MEDIUM:
		return SIZE_LARGE;
	    case SIZE_LARGE:
		return SIZE_LARGEPLUS;
	    case SIZE_LARGEPLUS:
		break;
	}
	assert(0);
	return SIZE_LARGEPLUS;
}

static
unsigned
sizeblocks(enum sizes sz)
{
	/*
	 * XXX for now hardwire things we know about SFS
	 */
#define BLOCKSIZE /*SFS_BLOCKSIZE*/ 512
	static const unsigned ndb = /*SFS_NDIRECT*/ 15;
	static const unsigned dbperidb = /*SFS_DBPERIDB*/ 128;

	switch (sz) {
	    case SIZE_ONE:
		/* one block; 512 bytes */
		return 1;
	    case SIZE_SMALL:
		/* fits in direct blocks only; 7.5K */
		return ndb;
	    case SIZE_MEDIUM:
		/* uses an indirect block; ~40K */
		return ndb + dbperidb / 2;
	    case SIZE_LARGE:
		/* uses a double-indirect block; 4.2M */
		return ndb + dbperidb + dbperidb * dbperidb / 2;
	    case SIZE_LARGEPLUS:
		/* requires a triple-indirect block; 8.5M */
		return ndb + dbperidb + dbperidb * dbperidb + dbperidb / 2;
	}
	assert(0);
	return 12;
}

static
unsigned
sizebytes(enum sizes sz)
{
	return BLOCKSIZE * sizeblocks(sz);
}

////////////////////////////////////////////////////////////
// common suboperations

static
void
file_randomwrite(struct file *f, enum sizes sz,
		 unsigned startskip, unsigned endskip)
{
	unsigned nblocks, nwrites, i;
	unsigned blocknum;
	off_t pos;

	nblocks = sizeblocks(sz);
	assert(nblocks > startskip + endskip);

	nwrites = nblocks/6;
	if (nwrites < 2) {
		nwrites = 2;
	}

	nblocks -= startskip + endskip;
	for (i=0; i<nwrites; i++) {
		blocknum = startskip + random() % nblocks;
		pos = (off_t)BLOCKSIZE * blocknum;
		op_write(f, pos, BLOCKSIZE);
	}
}

static
void
writeemptyfile(unsigned filenum)
{
	struct file *f;
	unsigned fake_testcode = 0; /* will not be used */

	f = op_open(fake_testcode, filenum, O_CREAT|O_EXCL);
	op_close(f);
}

static
void
writeoutfile(unsigned testcode, unsigned filenum, unsigned openflags,
	     enum sizes sz)
{
	struct file *f;

	f = op_open(testcode, filenum, openflags);
	op_write(f, 0, sizebytes(sz));
	op_close(f);
}

static
void
writenewfile(unsigned testcode, unsigned filenum, enum sizes sz)
{
	writeoutfile(testcode, filenum, O_CREAT|O_EXCL, sz);
}

static
void
writeholeyfile(unsigned testcode, unsigned filenum, enum sizes sz)
{
	struct file *f;
	unsigned openflags = O_CREAT|O_EXCL;

	f = op_open(testcode, filenum, openflags);
	op_write(f, 0, BLOCKSIZE);
	op_write(f, sizebytes(sz) - BLOCKSIZE, BLOCKSIZE);
	op_close(f);
}

/*
 * Standard subtree
 */
static
void
makesubtree(unsigned testcode, unsigned filenum)
{
	unsigned i;

	op_mkdir(filenum);
	op_chdir(filenum);
	for (i=0; i<7; i++) {
		if (i == 2 || i == 5) {
			op_mkdir(i);
		}
		else {
			writenewfile(testcode, i, SIZE_ONE);
		}
	}
	op_chdir(2);
	for (i=0; i<4; i++) {
		writenewfile(testcode, i+3, SIZE_ONE);
	}
	op_chdirup();
	op_chdir(5);
	for (i=0; i<5; i++) {
		if (i == 3) {
			op_mkdir(i+3);
		}
		else {
			writenewfile(testcode, i+3, SIZE_ONE);
		}
	}
	op_chdir(6);
	for (i=0; i<2; i++) {
		writenewfile(testcode, i+7, SIZE_ONE);
	}
	op_chdirup();
	op_chdirup();
	op_chdirup();
}

////////////////////////////////////////////////////////////
// writing

/*
 * Create and write out a file.
 */
void
wl_createwrite(const char *size)
{
	unsigned testcode = 1;
	enum sizes sz;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
}

/*
 * Rewrite an existing file.
 * (We have to create it and write it out, then sync, then rewrite.)
 */
void
wl_rewrite(const char *size)
{
	unsigned testcode = 2; /* and 3 */
	enum sizes sz;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	writeoutfile(testcode + 1, 0/*filenum*/, 0, sz);
}

/*
 * Do random updates to an existing file.
 * (Again, we have to create it first.)
 */
void
wl_randupdate(const char *size)
{
	unsigned testcode = 4; /* and 5 */
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	srandom(71654);
	f = op_open(testcode + 1, 0/*filenum*/, 0);
	file_randomwrite(f, sz, 0, 0);
	op_close(f);
}

/*
 * Truncate and rewrite an existing file.
 * (Again, we have to create it first.)
 */
void
wl_truncwrite(const char *size)
{
	unsigned testcode = 6; /* and 7 */
	enum sizes sz;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	writeoutfile(testcode, 0/*filenum*/, O_TRUNC, sz);
}

/*
 * Write a new file with a hole. We do this by writing the first block
 * and seeking, then writing the last block.
 */
void
wl_makehole(const char *size)
{
	unsigned testcode = 8;
	enum sizes sz;

	sz = strtosize(size);

	writeholeyfile(testcode, 0/*filenum*/, sz);
}

/*
 * Fill in (part of) a hole in an existing file.
 * (The file has to be created first.)
 */
void
wl_fillhole(const char *size)
{
	unsigned testcode = 9; /* and 10 */
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);

	writeholeyfile(testcode, 0/*filenum*/, sz);
	op_sync();
	srandom(51743);
	f = op_open(testcode + 1, 0/*filenum*/, 0);
	file_randomwrite(f, sz, 1, 1);
	op_close(f);
}

/*
 * Create an all-holes file with truncate and then fill part of it in.
 */
void
wl_truncfill(const char *size)
{
	unsigned testcode = 11;
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);

	f = op_open(testcode, 0/*filenum*/, O_CREAT|O_EXCL);
	op_truncate(f, sizebytes(sz));
	op_close(f);
	op_sync();
	srandom(52548);
	f = op_open(testcode, 0/*filenum*/, 0);
	file_randomwrite(f, sz, 0, 0);
	op_close(f);
}

/*
 * Append to an existing file.
 * (As usual we have to create the file first.)
 */
void
wl_append(const char *size)
{
	unsigned testcode = 11;
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	f = op_open(testcode + 1, 0/*filenum*/, 0);
	op_write(f, sizebytes(sz), BLOCKSIZE * 4);
	op_close(f);
}

////////////////////////////////////////////////////////////
// truncating

/*
 * Truncate an existing file to zero length.
 */
void
wl_trunczero(const char *size)
{
	unsigned testcode = 50;
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	f = op_open(testcode, 0/*filenum*/, 0);
	op_truncate(f, 0);
	op_close(f);
}

/*
 * Truncate an existing file to shorten it by one block.
 */
void
wl_trunconeblock(const char *size)
{
	unsigned testcode = 50;
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	f = op_open(testcode, 0/*filenum*/, 0);
	op_truncate(f, sizebytes(sz) - BLOCKSIZE);
	op_close(f);
}

/*
 * Truncate an existing file to the next smaller size category.
 */
void
wl_truncsmallersize(const char *size)
{
	unsigned testcode = 50;
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);
	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	f = op_open(testcode, 0/*filenum*/, 0);
	op_truncate(f, sizebytes(nextsmallersize(sz)));
	op_close(f);
}

/*
 * Truncate an existing file to the next larger size category.
 */
void
wl_trunclargersize(const char *size)
{
	unsigned testcode = 50;
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	f = op_open(testcode, 0/*filenum*/, 0);
	op_truncate(f, sizebytes(nextlargersize(sz)));
	op_close(f);
}

/*
 * Append to a file and then truncate it to zero without syncing.
 */
void
wl_appendandtrunczero(const char *size)
{
	unsigned testcode = 50;
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	f = op_open(testcode, 0/*filenum*/, 0);
	op_write(f, sizebytes(sz), BLOCKSIZE * 4);
	op_truncate(f, 0);
	op_close(f);
}

/*
 * Append to a file and then truncate part of what we wrote without
 * syncing.
 */
void
wl_appendandtruncpartly(const char *size)
{
	unsigned testcode = 50;
	enum sizes sz;
	struct file *f;

	sz = strtosize(size);

	writenewfile(testcode, 0/*filenum*/, sz);
	op_sync();
	f = op_open(testcode, 0/*filenum*/, 0);
	op_write(f, sizebytes(sz), BLOCKSIZE * 4);
	op_truncate(f, sizebytes(sz) + BLOCKSIZE * 2);
	op_close(f);
}

////////////////////////////////////////////////////////////
// creating

/*
 * Create a file. (This is meant to concentrate on the directory
 * operation, so create a one-block file only.)
 */
void
wl_mkfile(void)
{
	unsigned testcode = 100;

	writenewfile(testcode, 0/*filenum*/, SIZE_ONE);
}

/*
 * Create a directory.
 */
void
wl_mkdir(void)
{
	op_mkdir(0/*filenum*/);
}

/*
 * Create a bunch of (one-block) files.
 */
void
wl_mkmanyfile(void)
{
	unsigned testcode = 101;
	unsigned numfiles = 27;
	unsigned i;

	for (i=0; i<numfiles; i++) {
		writenewfile(testcode, i/*filenum*/, SIZE_ONE);
	}
}

/*
 * Create a bunch of directories.
 */
void
wl_mkmanydir(void)
{
	unsigned numdirs = 27;
	unsigned i;

	for (i=0; i<numdirs; i++) {
		op_mkdir(i/*filenum*/);
	}
}

/*
 * Create a canned tree of directories and files.
 */
#include <stdio.h>
static
void
wl_mktree_sub(unsigned testcode, unsigned depth)
{
	unsigned i, numthings = 4;

	for (i=0; i<numthings; i++) {
		if (i < depth) {
			writenewfile(testcode, i/*filenum*/, SIZE_ONE);
		}
		else {
			op_mkdir(i/*filenum*/);
			if (depth < numthings) {
				op_chdir(i/*filenum*/);
				wl_mktree_sub(testcode, depth + 1);
				op_chdirup();
			}
		}
	}
}

static
void
wl_rmtree_sub(unsigned depth)
{
	unsigned i, numthings = 4;

	for (i=0; i<numthings; i++) {
		if (i < depth) {
			op_unlink(i/*filenum*/);
		}
		else {
			if (depth < numthings) {
				op_chdir(i/*filenum*/);
				wl_rmtree_sub(depth + 1);
				op_chdirup();
			}
			op_rmdir(i/*filenum*/);
		}
	}
}

void
wl_mktree(void)
{
	unsigned testcode = 102;

	wl_mktree_sub(testcode, 0);
}

/*
 * Create a randomly generated tree of directories and files.
 */

static
void
mkrandtree_sub(unsigned testcode, unsigned depth,
	       unsigned *ct, unsigned numthings)
{
	unsigned numhere;

	numhere = 0;
	while (*ct < numthings) {
		switch (random() % 4) {
		    case 0:
			(*ct)++;
			op_mkdir(numhere/*filenum*/);
			op_chdir(numhere/*filenum*/);
			mkrandtree_sub(testcode, depth + 1, ct, numthings);
			op_chdirup();
			numhere++;
			break;
		    case 1:
			if (depth > 0) {
				return;
			}
			break;
		    case 2:
		    case 3:
			writenewfile(testcode, numhere/*filenum*/, SIZE_ONE);
			(*ct)++;
			numhere++;
			break;
		}
	}
}

static
void
rmrandtree_sub(unsigned depth, unsigned *ct, unsigned numthings)
{
	unsigned numhere;

	numhere = 0;
	while (*ct < numthings) {
		switch (random() % 4) {
		    case 0:
			(*ct)++;
			op_chdir(numhere/*filenum*/);
			rmrandtree_sub(depth + 1, ct, numthings);
			op_chdirup();
			op_rmdir(numhere/*filenum*/);
			numhere++;
			break;
		    case 1:
			if (depth > 0) {
				return;
			}
			break;
		    case 2:
		    case 3:
			op_unlink(numhere/*filenum*/);
			(*ct)++;
			numhere++;
			break;
		}
	}
}

void
wl_mkrandtree(const char *seed)
{
	unsigned testcode = 103;
	unsigned numthings, count;
	unsigned long seednum = getnum(seed);

	srandom(seednum);
	numthings = random() % 44 + 12;
	count = 0;

	mkrandtree_sub(testcode, 0, &count, numthings);
}

////////////////////////////////////////////////////////////
// deleting

/*
 * Delete a file. (We have to create the file first and sync it.)
 */
void
wl_rmfile(void)
{
	unsigned testcode = 150;

	writenewfile(testcode, 0/*filenum*/, SIZE_ONE);
	writeemptyfile(1/*filenum*/);
	op_sync();
	op_unlink(0/*filenum*/);
}

/*
 * Delete a directory. (We have to mkdir first and sync it.)
 */
void
wl_rmdir(void)
{
	op_mkdir(0/*filenum*/);
	writeemptyfile(1/*filenum*/);
	op_sync();
	op_rmdir(0/*filenum*/);
}

/*
 * Delete a file, with delayed reclaim.
 *
 * (Should there be a form of this that syncs after unlinking but
 * before closing?)
 */
void
wl_rmfiledelayed(void)
{
	unsigned testcode = 151;
	struct file *f;

	writenewfile(testcode, 0/*filenum*/, SIZE_ONE);
	writeemptyfile(1/*filenum*/);
	op_sync();
	f = op_open(testcode, 0/*filenum*/, 0);
	op_unlink(0/*filenum*/);
	op_close(f);
}

/*
 * Delete a file, with delayed reclaim, and append to the file before
 * reclaiming.
 */
void
wl_rmfiledelayedappend(void)
{
	unsigned testcode = 152;
	struct file *f;
	enum sizes sz = SIZE_SMALL;

	writenewfile(testcode, 0/*filenum*/, sz);
	writeemptyfile(1/*filenum*/);
	op_sync();
	f = op_open(testcode, 0/*filenum*/, 0);
	op_unlink(0/*filenum*/);
	op_write(f, sizebytes(sz), 6 * BLOCKSIZE);
	op_close(f);
}

/*
 * Delete a directory, with delayed reclaim.
 */
void
wl_rmdirdelayed(void)
{
	struct dir *d;

	op_mkdir(0/*filenum*/);
	writeemptyfile(1/*filenum*/);
	op_sync();
	d = op_opendir(0/*filenum*/);
	op_rmdir(0/*filenum*/);
	op_closedir(d);
}

/*
 * Delete many files.
 */
void
wl_rmmanyfile(void)
{
	unsigned testcode = 153;
	unsigned numfiles = 27;
	unsigned i;

	for (i=0; i<numfiles; i++) {
		writenewfile(testcode, i/*filenum*/, SIZE_ONE);
	}
	writeemptyfile(numfiles/*filenum*/);
	op_sync();
	for (i=0; i<numfiles; i++) {
		op_unlink(i/*filenum*/);
	}
}

/*
 * Delete many files, with delayed reclaim.
 */
void
wl_rmmanyfiledelayed(void)
{
	unsigned testcode = 154;
	unsigned numfiles = 27;
	struct file *files[27];
	unsigned i;

	for (i=0; i<numfiles; i++) {
		writenewfile(testcode, i/*filenum*/, SIZE_ONE);
	}
	writeemptyfile(numfiles/*filenum*/);
	op_sync();
	for (i=0; i<numfiles; i++) {
		files[i] = op_open(testcode, i/*filenum*/, 0);
	}
	for (i=0; i<numfiles; i++) {
		op_unlink(i/*filenum*/);
	}
	for (i=0; i<numfiles; i++) {
		op_close(files[i]);
	}
}

/*
 * Delete many files, with delayed reclaim, and append to the files
 * before reclaiming. For variety, we append to half the files first,
 * reclaim half of those, then append one and reclaim two until we
 * finish.
 */
void
wl_rmmanyfiledelayedandappend(void)
{
	unsigned testcode = 155;
	unsigned numfiles = 27;
	struct file *files[27];
	enum sizes sz = SIZE_SMALL;
	unsigned i, j;

	for (i=0; i<numfiles; i++) {
		writenewfile(testcode, i/*filenum*/, sz);
	}
	writeemptyfile(numfiles/*filenum*/);
	op_sync();
	for (i=0; i<numfiles; i++) {
		files[i] = op_open(testcode, i/*filenum*/, 0);
	}
	for (i=0; i<numfiles; i++) {
		op_unlink(i/*filenum*/);
	}

	for (i=0; i<numfiles/2; i++) {
		op_write(files[i], sizebytes(sz), 6 * BLOCKSIZE);
	}
	for (j=0; j<numfiles/4; j++) {
		op_close(files[j]);
	}
	assert(j<=i);
	while (j<numfiles) {
		assert(j<=i);
		if (i<numfiles) {
			op_write(files[i++], sizebytes(sz), 6 * BLOCKSIZE);
		}
		op_close(files[j++]);
		if (j < i) {
			op_close(files[j++]);
		}
	}
	assert(i==numfiles);
	assert(j==i);
}

/*
 * Delete many directories.
 */
void
wl_rmmanydir(void)
{
	unsigned numdirs = 27;
	unsigned i;

	for (i=0; i<numdirs; i++) {
		op_mkdir(i/*filenum*/);
	}
	writeemptyfile(numdirs/*filenum*/);
	op_sync();
	for (i=0; i<numdirs; i++) {
		op_rmdir(i/*filenum*/);
	}
}

/*
 * Delete many directories, with delayed reclaim.
 */
void
wl_rmmanydirdelayed(void)
{
	unsigned numdirs = 27;
	struct dir *dirs[27];
	unsigned i;

	for (i=0; i<numdirs; i++) {
		op_mkdir(i/*filenum*/);
	}
	writeemptyfile(numdirs/*filenum*/);
	op_sync();
	for (i=0; i<numdirs; i++) {
		dirs[i] = op_opendir(i/*filenum*/);
	}
	for (i=0; i<numdirs; i++) {
		op_rmdir(i/*filenum*/);
	}
	for (i=0; i<numdirs; i++) {
		op_closedir(dirs[i]);
	}
}

/*
 * Delete a canned tree of directories and files.
 */
void
wl_rmtree(void)
{
	unsigned testcode = 156;

	wl_mktree_sub(testcode, 0);
	op_sync();
	wl_rmtree_sub(0);
}

/*
 * Delete a randomly generated tree of directories and files.
 */
void
wl_rmrandtree(const char *seed)
{
	unsigned testcode = 157;
	unsigned long seednum = getnum(seed);
	unsigned numthings, numthings2, count;

	srandom(seednum);
	numthings = random() % 44 + 12;
	count = 0;

	mkrandtree_sub(testcode, 0, &count, numthings);
	op_sync();

	srandom(seednum);
	numthings2 = random() % 44 + 12;
	assert(numthings == numthings2);
	count = 0;

	rmrandtree_sub(0, &count, numthings);
}

////////////////////////////////////////////////////////////
// link

/*
 * (Hard) link one file.
 */
void
wl_linkfile(void)
{
	unsigned testcode = 200;

	writenewfile(testcode, 0/*filenum*/, SIZE_ONE);
	op_sync();
	op_link(0/*oldname*/, 1/*newname*/);
}

/*
 * (Hard) link many files.
 */
void
wl_linkmanyfile(void)
{
	unsigned testcode = 201;
	unsigned i, numfiles = 14;

	for (i=0; i<numfiles; i++) {
		writenewfile(testcode, i/*filenum*/, SIZE_ONE);
	}
	op_sync();
	for (i=0; i<numfiles; i++) {
		op_link(i, numfiles+i);
	}
}

/*
 * Unlink one file (not the last link).
 */
void
wl_unlinkfile(void)
{
	unsigned testcode = 202;

	writenewfile(testcode, 0/*filenum*/, SIZE_ONE);
	op_link(0/*oldname*/, 1/*newname*/);
	writeemptyfile(2/*filenum*/);
	op_sync();
	op_unlink(1/*newname*/);
}

/*
 * Unlink many files (not the last link).
 */
void
wl_unlinkmanyfile(void)
{
	unsigned testcode = 203;
	unsigned i, numfiles = 14;

	for (i=0; i<numfiles; i++) {
		writenewfile(testcode, i/*filenum*/, SIZE_ONE);
	}
	for (i=0; i<numfiles; i++) {
		op_link(i, numfiles+i);
	}
	writeemptyfile(numfiles*2/*filenum*/);
	op_sync();
	for (i=0; i<numfiles; i++) {
		/* mix it up, just for fun */
		if (i<numfiles/2) {
			op_unlink(i);
		}
		else {
			op_unlink(numfiles+i);
		}
	}
}

/*
 * Hard link and unlink the same file without syncing.
 */
void
wl_linkunlinkfile(void)
{
	unsigned testcode = 204;

	writenewfile(testcode, 0/*filenum*/, SIZE_ONE);
	op_sync();
	op_link(0/*oldname*/, 1/*newname*/);
	op_unlink(1/*newname*/);
}

////////////////////////////////////////////////////////////
// rename

/*
 * Rename one file.
 */
void
wl_renamefile(void)
{
	unsigned testcode = 250;

	writenewfile(testcode, 0/*filenum*/, SIZE_ONE);
	writeemptyfile(2/*filenum*/);
	op_sync();
	op_rename(0/*oldname*/, 1/*newname*/);
}

/*
 * Rename one directory.
 */
void
wl_renamedir(void)
{
	op_mkdir(0/*filenum*/);
	writeemptyfile(2/*filenum*/);
	op_sync();
	op_rename(0/*oldname*/, 1/*newname*/);
}

/*
 * Rename a subtree.
 */
void
wl_renamesubtree(void)
{
	unsigned testcode = 251;

	makesubtree(testcode, 0/*filenum*/);
	writeemptyfile(2/*filenum*/);
	op_sync();
	op_rename(0/*oldname*/, 1/*newname*/);
}

/*
 * Rename one file across directories.
 */
void
wl_renamexdfile(void)
{
	unsigned testcode = 252;

	op_mkdir(0/*dir*/);
	op_mkdir(1/*dir*/);
	op_chdir(0/*dir*/);
	writenewfile(testcode, 2/*filenum*/, SIZE_ONE);
	op_chdirup();
	op_sync();
	op_renamexd(0/*olddir*/, 2/*oldname*/, 1/*newdir*/, 3/*newname*/);
}

/*
 * Rename one directory across directories.
 */
void
wl_renamexddir(void)
{
	op_mkdir(0/*dir*/);
	op_mkdir(1/*dir*/);
	op_chdir(0/*dir*/);
	op_mkdir(2/*filenum*/);
	op_chdirup();
	op_sync();
	op_renamexd(0/*olddir*/, 2/*oldname*/, 1/*newdir*/, 3/*newname*/);
}

/*
 * Rename a subtree across directories.
 */
void
wl_renamexdsubtree(void)
{
	unsigned testcode = 253;

	op_mkdir(0/*dir*/);
	op_mkdir(1/*dir*/);
	op_chdir(0/*dir*/);
	makesubtree(testcode, 2/*filenum*/);
	op_chdirup();
	op_sync();
	op_renamexd(0/*olddir*/, 2/*oldname*/, 1/*newdir*/, 3/*newname*/);
}

/*
 * Rename many files.
 */
void
wl_renamemanyfile(void)
{
	unsigned testcode = 254;
	unsigned i, numfiles = 14;

	for (i=0; i<numfiles; i++) {
		writenewfile(testcode, i/*filenum*/, SIZE_ONE);
	}
	writeemptyfile(numfiles*2/*filenum*/);
	op_sync();
	for (i=0; i<numfiles; i++) {
		op_rename(i, numfiles+i);
	}
}

/*
 * Rename many directories.
 */
void
wl_renamemanydir(void)
{
	unsigned i, numdirs = 14;

	for (i=0; i<numdirs; i++) {
		op_mkdir(i/*filenum*/);
	}
	writeemptyfile(numdirs*2/*filenum*/);
	op_sync();
	for (i=0; i<numdirs; i++) {
		op_rename(i, numdirs+i);
	}
}

/*
 * Rename many subtrees.
 */
void
wl_renamemanysubtree(void)
{
	unsigned testcode = 255;
	unsigned i, numtrees = 14;

	for (i=0; i<numtrees; i++) {
		makesubtree(testcode, i/*filenum*/);
	}
	writeemptyfile(numtrees*2/*filenum*/);
	op_sync();
	for (i=0; i<numtrees; i++) {
		op_rename(i, numtrees+i);
	}
}

////////////////////////////////////////////////////////////
// combo ops

/*
 * Write out a new version of a file and rename it into place.
 */
void
wl_copyandrename(void)
{
	unsigned testcode = 300; /* and 301 */
	enum sizes sz = SIZE_MEDIUM;

	writenewfile(testcode, 0/*filenum*/, sz);
	writeemptyfile(2/*filenum*/);
	op_sync();
	writenewfile(testcode+1, 1/*filenum*/, sz);
	op_rename(1, 0);
}

/*
 * Simulate an untar: create and populate a subtree.
 */
void
wl_untar(void)
{
	unsigned testcode = 302;

	makesubtree(testcode, 0/*filenum*/);
}

/*
 * Simulate a compile: create .o files next to .c files, then
 * a program file.
 */
void
wl_compile(void)
{
#if 0 /* notyet -- setfilesuffix() is not implemented */
	unsigned testcode = 303; /* and 304 */
	unsigned i, numfiles = 27;

	setfilesuffix(".c");
	for (i=0; i<numfiles; i++) {
		writenewfile(testcode, i/*filenum*/, SIZE_SMALL);
	}
	op_sync();
	setfilesuffix(".o");
	for (i=0; i<numfiles; i++) {
		writenewfile(testcode + 1, i/*filenum*/, SIZE_SMALL);
	}
	writenewfile(testcode + 1, numfiles, SIZE_MEDIUM);
#endif
	errx(1, "The compile workload isn't implemented yet.");
}

/*
 * Simulate the horrible things that cvs update does.
 */
void
wl_cvsupdate(void)
{
	//unsigned testcode = 305;

	errx(1, "This one isn't implementd yet.");
}

////////////////////////////////////////////////////////////
// Randomized op sequences

static
void
createfiles(unsigned testcode, unsigned num, enum sizes sz)
{
	unsigned i;

	for (i=0; i<num; i++) {
		writenewfile(testcode, i/*filenum*/, sz);
	}
}

static
void
openfiles(unsigned testcode,
	  struct file **files, unsigned opennum, unsigned totnum)
{
	unsigned i;

	for (i=0; i<opennum; i++) {
		files[i] = op_open(testcode + 1, i/*filenum*/, 0);
	}
	for (; i<totnum; i++) {
		files[i] = NULL;
	}
}

static
void
randwrite(struct file *file)
{
	off_t maxpos, pos, len;

	maxpos = sizebytes(SIZE_MEDIUM) + 49152;
	/* pick a position and length */
	pos = random() % maxpos;
	len = random() % 12000 + 200;
	op_write(file, pos, len);
}

static
void
randtruncate(struct file *file)
{
	op_truncate(file, random() % sizebytes(SIZE_MEDIUM));
}

/*
 * Sequence of randomly chosen writes to files.
 */
void
wl_writefileseq(const char *seed)
{
	unsigned testcode = 350; /* and 351 */
	unsigned numfiles = 27;
	struct file *files[27];
	unsigned filenum;
	unsigned i, numops;
	unsigned long seednum = getnum(seed);

	srandom(seednum);

	/* create half the files beforehand */
	createfiles(testcode, numfiles/2, SIZE_MEDIUM);
	op_sync();
	openfiles(testcode, files, numfiles/2, numfiles);

	/* do 100-200 writes */
	numops = random() % 100 + 100;
	for (i=0; i<numops; i++) {
		/* pick a file */
		filenum = random() % numfiles;
		/* open it if it isn't yet */
		if (files[filenum] == NULL) {
			files[filenum] = op_open(testcode + 1, filenum,
						 O_CREAT|O_EXCL);
		}
		randwrite(files[filenum]);
	}
}

/*
 * Sequence of randomly chosen writes to files, interspersed with
 * truncates.
 */
void
wl_writetruncseq(const char *seed)
{
	unsigned testcode = 352; /* and 353 */
	unsigned numfiles = 27;
	struct file *files[27];
	unsigned filenum;
	unsigned i, numops;
	unsigned long seednum = getnum(seed);

	srandom(seednum);

	/* create half the files beforehand */
	createfiles(testcode, numfiles/2, SIZE_MEDIUM);
	op_sync();
	openfiles(testcode, files, numfiles/2, numfiles);

	/* do 100-200 operations */
	numops = random() % 100 + 100;
	for (i=0; i<numops; i++) {
		/* pick a file */
		filenum = random() % numfiles;
		/* open it if it isn't yet */
		if (files[filenum] == NULL) {
			files[filenum] = op_open(testcode + 1, filenum,
						 O_CREAT|O_EXCL);
		}
		if (random() % 5 == 0) {
			randtruncate(files[filenum]);
		}
		else {
			randwrite(files[filenum]);
		}
	}
}

/*
 * Sequence of randomly chosen create and delete operations.
 */
void
wl_mkrmseq(const char *seed)
{
	unsigned testcode = 354;
	unsigned numfiles = 27;
	unsigned exists[27];
	unsigned filenum;
	unsigned i, numops;
	unsigned long seednum = getnum(seed);

	srandom(seednum);

	for (i=0; i<numfiles; i++) {
		exists[i] = 0;
	}

	/* do 100-200 operations */
	numops = random() % 100 + 100;
	for (i=0; i<numops; i++) {
		filenum = random() % numfiles;
		if (exists[filenum]) {
			op_unlink(filenum);
			exists[filenum] = 0;
		}
		else {
			writenewfile(testcode, filenum, SIZE_ONE);
			exists[filenum] = 1;
		}
	}
}

/*
 * Sequence of randomly chosen link and unlink operations.
 */
void
wl_linkunlinkseq(const char *seed)
{
	unsigned testcode = 355;
	unsigned numfiles = 14;
	unsigned exists[14];
	unsigned filenum;
	unsigned i, numops;
	unsigned long seednum = getnum(seed);

	srandom(seednum);

	for (i=0; i<numfiles; i++) {
		exists[i] = 0;
	}

	for (i=0; i<numfiles; i++) {
		writenewfile(testcode, i, SIZE_ONE);
	}
	op_sync();

	/* do 100-200 operations */
	numops = random() % 100 + 100;
	for (i=0; i<numops; i++) {
		filenum = random() % numfiles;
		if (exists[filenum]) {
			op_unlink(filenum + numfiles);
			exists[filenum] = 0;
		}
		else {
			op_link(random() % numfiles, filenum + numfiles);
			exists[filenum] = 1;
		}
	}
}

/*
 * Sequence of randomly chosen renames.
 */
void
wl_renameseq(const char *seed)
{
	unsigned testcode = 356;
	unsigned numfiles = 27;
	unsigned exists[27];
	unsigned filenum1, filenum2;
	unsigned i, ct, numops;
	unsigned long seednum = getnum(seed);

	srandom(seednum);

	for (i=0; i<numfiles; i++) {
		exists[i] = 0;
	}

	for (i=0; i<numfiles/3; i++) {
		writenewfile(testcode, i, SIZE_ONE);
		exists[i] = 1;
	}
	op_sync();

	/* do 100-200 operations */
	numops = random() % 100 + 100;
	ct = 0;
	while (ct < numops) {
		filenum1 = random() % numfiles;
		filenum2 = random() % numfiles;
		if (exists[filenum1] && !exists[filenum2]) {
			op_rename(filenum1, filenum2);
			exists[filenum1] = 0;
			exists[filenum2] = 1;
			ct++;
		}
	}
}

/*
 * Sequence of randomly chosen arbitrary directory operations.
 */

#define ISNT	0
#define IS_FILE 1
#define IS_DIR  2

static
bool
randop(unsigned testcode, unsigned *exists, unsigned numfiles,
       bool dofileops, struct file **files)
{
	enum sizes sz;
	unsigned filenum, filenum2;

	switch (random() % (dofileops ? 8 : 6)) {
	    case 0:
		/* create a file */
		filenum = random() % numfiles;
		if (exists[filenum] == ISNT) {
			if (dofileops) {
				sz = randsize();
			}
			else {
				sz = SIZE_ONE;
			}
			writenewfile(testcode, filenum, sz);
			exists[filenum] = IS_FILE;
			return true;
		}
		break;
	    case 1:
		/* make a directory */
		filenum = random() % numfiles;
		if (exists[filenum] == ISNT) {
			op_mkdir(filenum);
			exists[filenum] = IS_DIR;
			return true;
		}
		break;
	    case 2:
#if 0 /* XXX currently no link() in OS/161 */
		/* hardlink a file */
		filenum = random() % numfiles;
		filenum2 = random() % numfiles;
		if (exists[filenum] == IS_FILE &&
		    exists[filenum2] == ISNT) {
			op_link(filenum, filenum2);
			exists[filenum2] = IS_FILE;
			return true;
		}
#endif
		break;
	    case 3:
		/* unlink a file */
		filenum = random() % numfiles;
		if (exists[filenum] == IS_FILE) {
			op_unlink(filenum);
			exists[filenum] = ISNT;
			if (files != NULL && files[filenum] != NULL) {
				op_close(files[filenum]);
				files[filenum] = NULL;
			}
			return true;
		}
		break;
	    case 4:
		/* rmdir a dir */
		filenum = random() % numfiles;
		if (exists[filenum] == IS_DIR) {
			op_rmdir(filenum);
			exists[filenum] = ISNT;
			return true;
		}
		break;
	    case 5:
		/* rename something */
		filenum = random() % numfiles;
		filenum2 = random() % numfiles;

		/* XXX currently something in check.c horks on this case */
		if (filenum == filenum2) {
			break;
		}

		if (exists[filenum] != ISNT &&
		    (exists[filenum2] == ISNT ||
		     exists[filenum2] == exists[filenum])) {
			op_rename(filenum, filenum2);
			if (filenum != filenum2) {
				exists[filenum2] = exists[filenum];
				exists[filenum] = ISNT;
			}
			return true;
		}
		break;
	    case 6:
		/* truncate something */

		assert(dofileops);
		assert(files != NULL);

		filenum = random() % numfiles;
		if (exists[filenum] == IS_FILE) {
			if (files[filenum] == NULL) {
				files[filenum] = op_open(testcode, filenum, 0);
			}
			randtruncate(files[filenum]);
			return true;
		}
		break;
	    case 7:
		/* write to something */

		assert(dofileops);
		assert(files != NULL);

		filenum = random() % numfiles;
		if (exists[filenum] == IS_FILE) {
			if (files[filenum] == NULL) {
				files[filenum] = op_open(testcode, filenum, 0);
			}
			randwrite(files[filenum]);
			return true;
		}
		break;
	}
	return false;
}

static
void
prep(unsigned testcode, unsigned *exists, unsigned numfiles,
     bool dofileops, struct file **files)
{
	unsigned i;

	for (i=0; i<numfiles; i++) {
		exists[i] = ISNT;
	}

	for (i=0; i<numfiles/4; i++) {
		writenewfile(testcode, i, SIZE_ONE);
		exists[i] = IS_FILE;
	}
	for (i=0; i<numfiles/3; i++) {
		op_mkdir(numfiles/4 + i);
		exists[numfiles/4 + i] = IS_DIR;
	}

	if (dofileops) {
		for (i=0; i<numfiles; i++) {
			files[i] = NULL;
		}
	}
}

void
wl_diropseq(const char *seed)
{
	unsigned testcode = 357;
	unsigned numfiles = 27;
	unsigned exists[27];
	unsigned ct, numops;
	unsigned long seednum = getnum(seed);

	srandom(seednum);

	prep(testcode, exists, numfiles, false, NULL);
	op_sync();

	/* do 100-200 operations */
	numops = random() % 100 + 100;
	ct = 0;
	while (ct < numops) {
		if (randop(testcode, exists, numfiles, false, NULL)) {
			ct++;
		}
	}
}

/*
 * Sequence of randomly chosen arbitrary operations.
 */
void
wl_genseq(const char *seed)
{
	unsigned testcode = 358;
	unsigned numfiles = 27;
	unsigned exists[27];
	struct file *files[27];
	unsigned ct, numops;
	unsigned long seednum = getnum(seed);

	srandom(seednum);

	prep(testcode, exists, numfiles, true, files);
	op_sync();

	/* do 100-200 operations */
	numops = random() % 100 + 100;
	ct = 0;
	while (ct < numops) {
		if (randop(testcode, exists, numfiles, true, files)) {
			ct++;
		}
	}
}
