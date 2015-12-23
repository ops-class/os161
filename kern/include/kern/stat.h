/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008
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

#ifndef _KERN_STAT_H_
#define _KERN_STAT_H_

/*
 * The stat structure, for returning file information via stat(),
 * fstat(), and lstat().
 *
 * Fields corresponding to things you aren't implementing should be
 * set to zero.
 *
 * The file types are in kern/stattypes.h.
 */
struct stat {
	/* Essential fields */
	off_t st_size;		/* file size in bytes */
	mode_t st_mode;		/* file type and protection mode */
	nlink_t st_nlink;	/* number of hard links */
	blkcnt_t st_blocks;	/* number of blocks file is using */

 	/* Identity */
	dev_t st_dev;           /* device object lives on */
	ino_t st_ino;           /* inode number (serial number) of object */
	dev_t st_rdev;          /* device object is (if a device) */

	/* Timestamps */
	time_t st_atime;        /* last access time: seconds */
	time_t st_ctime;        /* inode change time: seconds */
	time_t st_mtime;        /* modification time: seconds */
	__u32 st_atimensec;     /* last access time: nanoseconds */
	__u32 st_ctimensec;     /* inode change time: nanoseconds */
	__u32 st_mtimensec;     /* modification time: nanoseconds */

	/* Permissions (also st_mode) */
	uid_t st_uid;           /* owner */
	gid_t st_gid;           /* group */

	/* Other */
	__u32 st_gen;           /* file generation number (root only) */
	blksize_t st_blksize;   /* recommended I/O block size */
};

#endif /* _KERN_STAT_H_ */
