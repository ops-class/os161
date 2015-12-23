/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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

#ifndef _FS_H_
#define _FS_H_

struct vnode; /* in vnode.h */


/*
 * Abstract file system. (Or device accessible as a file.)
 *
 * fs_data is a pointer to filesystem-specific data.
 */

struct fs {
	void *fs_data;
	const struct fs_ops *fs_ops;
};

/*
 * Abstraction operations on a file system:
 *
 *      fsop_sync       - Flush all dirty buffers to disk.
 *      fsop_getvolname - Return volume name of filesystem.
 *      fsop_getroot    - Return root vnode of filesystem.
 *      fsop_unmount    - Attempt unmount of filesystem.
 *
 * fsop_getvolname may return NULL on filesystem types that don't
 * support the concept of a volume name. The string returned is
 * assumed to point into the filesystem's private storage and live
 * until unmount time.
 *
 * If the volume name changes on the fly, there is no way at present
 * to make sure such changes don't cause name conflicts. So it probably
 * should be considered fixed.
 *
 * fsop_getroot should increment the refcount of the vnode returned.
 * It should not ever return NULL.
 *
 * If fsop_unmount returns an error, the filesystem stays mounted, and
 * consequently the struct fs instance should remain valid. On success,
 * however, the filesystem object and all storage associated with the
 * filesystem should have been discarded/released.
 */
struct fs_ops {
	int           (*fsop_sync)(struct fs *);
	const char   *(*fsop_getvolname)(struct fs *);
	int           (*fsop_getroot)(struct fs *, struct vnode **);
	int           (*fsop_unmount)(struct fs *);
};

/*
 * Macros to shorten the calling sequences.
 */
#define FSOP_SYNC(fs)        ((fs)->fs_ops->fsop_sync(fs))
#define FSOP_GETVOLNAME(fs)  ((fs)->fs_ops->fsop_getvolname(fs))
#define FSOP_GETROOT(fs, ret) ((fs)->fs_ops->fsop_getroot(fs, ret))
#define FSOP_UNMOUNT(fs)     ((fs)->fs_ops->fsop_unmount(fs))

/* Initialization functions for builtin fake file systems. */
void semfs_bootstrap(void);


#endif /* _FS_H_ */
