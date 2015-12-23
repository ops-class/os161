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

#ifndef _KERN_SFS_H_
#define _KERN_SFS_H_


/*
 * SFS definitions visible to userspace. This covers the on-disk format
 * and is used by tools that work on SFS volumes, such as mksfs.
 */

#define SFS_MAGIC         0xabadf001    /* magic number identifying us */
#define SFS_BLOCKSIZE     512           /* size of our blocks */
#define SFS_VOLNAME_SIZE  32            /* max length of volume name */
#define SFS_NDIRECT       15            /* # of direct blocks in inode */
#define SFS_NINDIRECT     1             /* # of indirect blocks in inode */
#define SFS_NDINDIRECT    0             /* # of 2x indirect blocks in inode */
#define SFS_NTINDIRECT    0             /* # of 3x indirect blocks in inode */
#define SFS_DBPERIDB      128           /* # direct blks per indirect blk */
#define SFS_NAMELEN       60            /* max length of filename */
#define SFS_SUPER_BLOCK   0             /* block the superblock lives in */
#define SFS_FREEMAP_START 2             /* 1st block of the freemap */
#define SFS_NOINO         0             /* inode # for free dir entry */
#define SFS_ROOTDIR_INO   1             /* loc'n of the root dir inode */

/* Number of bits in a block */
#define SFS_BITSPERBLOCK (SFS_BLOCKSIZE * CHAR_BIT)

/* Utility macro */
#define SFS_ROUNDUP(a,b)       ((((a)+(b)-1)/(b))*b)

/* Size of free block bitmap (in bits) */
#define SFS_FREEMAPBITS(nblocks) SFS_ROUNDUP(nblocks, SFS_BITSPERBLOCK)

/* Size of free block bitmap (in blocks) */
#define SFS_FREEMAPBLOCKS(nblocks)  (SFS_FREEMAPBITS(nblocks)/SFS_BITSPERBLOCK)

/* File types for sfi_type */
#define SFS_TYPE_INVAL    0       /* Should not appear on disk */
#define SFS_TYPE_FILE     1
#define SFS_TYPE_DIR      2

/*
 * On-disk superblock
 */
struct sfs_superblock {
	uint32_t sb_magic;		/* Magic number; should be SFS_MAGIC */
	uint32_t sb_nblocks;			/* Number of blocks in fs */
	char sb_volname[SFS_VOLNAME_SIZE];	/* Name of this volume */
	uint32_t reserved[118];			/* unused, set to 0 */
};

/*
 * On-disk inode
 */
struct sfs_dinode {
	uint32_t sfi_size;			/* Size of this file (bytes) */
	uint16_t sfi_type;			/* One of SFS_TYPE_* above */
	uint16_t sfi_linkcount;			/* # hard links to this file */
	uint32_t sfi_direct[SFS_NDIRECT];	/* Direct blocks */
	uint32_t sfi_indirect;			/* Indirect block */
	uint32_t sfi_waste[128-3-SFS_NDIRECT];	/* unused space, set to 0 */
};

/*
 * On-disk directory entry
 */
struct sfs_direntry {
	uint32_t sfd_ino;			/* Inode number */
	char sfd_name[SFS_NAMELEN];		/* Filename */
};


#endif /* _KERN_SFS_H_ */
