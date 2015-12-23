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

#ifndef SFS_H
#define SFS_H

/*
 * SFS operations. This module provides functions for reading and
 * writing SFS structures, and knows under the covers how to byte-swap
 * them. It also provides utility operations on the SFS structures.
 */

#include <stdint.h>

struct sfs_superblock;
struct sfs_dinode;
struct sfs_direntry;

/* Call this before anything else in this module */
void sfs_setup(void);

/*
 * Read and write ops for SFS structures
 */

/* superblock */
void sfs_readsb(uint32_t blocknum, struct sfs_superblock *sb);
void sfs_writesb(uint32_t blocknum, struct sfs_superblock *sb);

/* freemap blocks; whichblock is the freemap block number (starts at 0) */
void sfs_readfreemapblock(uint32_t whichblock, uint8_t *bits);
void sfs_writefreemapblock(uint32_t whichblock, uint8_t *bits);

/* inode */
void sfs_readinode(uint32_t inum, struct sfs_dinode *sfi);
void sfs_writeinode(uint32_t inum, struct sfs_dinode *sfi);

/* indirect block (any indirection level) */
void sfs_readindirect(uint32_t blocknum, uint32_t *entries);
void sfs_writeindirect(uint32_t blocknum, uint32_t *entries);

/* directory - ND should be the number of directory entries D points to */
void sfs_readdir(struct sfs_dinode *sfi, struct sfs_direntry *d, unsigned nd);
void sfs_writedir(const struct sfs_dinode *sfi,
		  struct sfs_direntry *d, unsigned nd);

/* Try to add an entry to a directory. */
int sfsdir_tryadd(struct sfs_direntry *d, int nd,
		  const char *name, uint32_t ino);

/* Sort a directory by creating a permutation vector. */
void sfsdir_sort(struct sfs_direntry *d, unsigned nd, int *vector);


#endif /* SFS_H */
