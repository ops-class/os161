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

#ifndef _SFSPRIVATE_H_
#define _SFSPRIVATE_H_

#include <uio.h> /* for uio_rw */


/* ops tables (in sfs_vnops.c) */
extern const struct vnode_ops sfs_fileops;
extern const struct vnode_ops sfs_dirops;

/* Macro for initializing a uio structure */
#define SFSUIO(iov, uio, ptr, block, rw) \
    uio_kinit(iov, uio, ptr, SFS_BLOCKSIZE, ((off_t)(block))*SFS_BLOCKSIZE, rw)


/* Functions in sfs_balloc.c */
int sfs_balloc(struct sfs_fs *sfs, daddr_t *diskblock);
void sfs_bfree(struct sfs_fs *sfs, daddr_t diskblock);
int sfs_bused(struct sfs_fs *sfs, daddr_t diskblock);

/* Functions in sfs_bmap.c */
int sfs_bmap(struct sfs_vnode *sv, uint32_t fileblock, bool doalloc,
		daddr_t *diskblock);
int sfs_itrunc(struct sfs_vnode *sv, off_t len);

/* Functions in sfs_dir.c */
int sfs_dir_findname(struct sfs_vnode *sv, const char *name,
		uint32_t *ino, int *slot, int *emptyslot);
int sfs_dir_link(struct sfs_vnode *sv, const char *name, uint32_t ino,
		int *slot);
int sfs_dir_unlink(struct sfs_vnode *sv, int slot);
int sfs_lookonce(struct sfs_vnode *sv, const char *name,
		struct sfs_vnode **ret,
		int *slot);

/* Functions in sfs_inode.c */
int sfs_sync_inode(struct sfs_vnode *sv);
int sfs_reclaim(struct vnode *v);
int sfs_loadvnode(struct sfs_fs *sfs, uint32_t ino, int forcetype,
		struct sfs_vnode **ret);
int sfs_makeobj(struct sfs_fs *sfs, int type, struct sfs_vnode **ret);
int sfs_getroot(struct fs *fs, struct vnode **ret);

/* Functions in sfs_io.c */
int sfs_readblock(struct sfs_fs *sfs, daddr_t block, void *data, size_t len);
int sfs_writeblock(struct sfs_fs *sfs, daddr_t block, void *data, size_t len);
int sfs_io(struct sfs_vnode *sv, struct uio *uio);
int sfs_metaio(struct sfs_vnode *sv, off_t pos, void *data, size_t len,
	       enum uio_rw rw);


#endif /* _SFSPRIVATE_H_ */
