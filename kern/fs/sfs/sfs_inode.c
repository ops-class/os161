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
 * Inode-level operations and vnode/inode lifecycle logic.
 */
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <vfs.h>
#include <sfs.h>
#include "sfsprivate.h"


/*
 * Write an on-disk inode structure back out to disk.
 */
int
sfs_sync_inode(struct sfs_vnode *sv)
{
	struct sfs_fs *sfs = sv->sv_absvn.vn_fs->fs_data;
	int result;

	if (sv->sv_dirty) {
		result = sfs_writeblock(sfs, sv->sv_ino, &sv->sv_i,
					sizeof(sv->sv_i));
		if (result) {
			return result;
		}
		sv->sv_dirty = false;
	}
	return 0;
}

/*
 * Called when the vnode refcount (in-memory usage count) hits zero.
 *
 * This function should try to avoid returning errors other than EBUSY.
 */
int
sfs_reclaim(struct vnode *v)
{
	struct sfs_vnode *sv = v->vn_data;
	struct sfs_fs *sfs = v->vn_fs->fs_data;
	unsigned ix, i, num;
	int result;

	vfs_biglock_acquire();

	/*
	 * Make sure someone else hasn't picked up the vnode since the
	 * decision was made to reclaim it. (You must also synchronize
	 * this with sfs_loadvnode.)
	 */
	spinlock_acquire(&v->vn_countlock);
	if (v->vn_refcount != 1) {

		/* consume the reference VOP_DECREF gave us */
		KASSERT(v->vn_refcount>1);
		v->vn_refcount--;

		spinlock_release(&v->vn_countlock);
		vfs_biglock_release();
		return EBUSY;
	}
	spinlock_release(&v->vn_countlock);

	/* If there are no on-disk references to the file either, erase it. */
	if (sv->sv_i.sfi_linkcount == 0) {
		result = sfs_itrunc(sv, 0);
		if (result) {
			vfs_biglock_release();
			return result;
		}
	}

	/* Sync the inode to disk */
	result = sfs_sync_inode(sv);
	if (result) {
		vfs_biglock_release();
		return result;
	}

	/* If there are no on-disk references, discard the inode */
	if (sv->sv_i.sfi_linkcount==0) {
		sfs_bfree(sfs, sv->sv_ino);
	}

	/* Remove the vnode structure from the table in the struct sfs_fs. */
	num = vnodearray_num(sfs->sfs_vnodes);
	ix = num;
	for (i=0; i<num; i++) {
		struct vnode *v2 = vnodearray_get(sfs->sfs_vnodes, i);
		struct sfs_vnode *sv2 = v2->vn_data;
		if (sv2 == sv) {
			ix = i;
			break;
		}
	}
	if (ix == num) {
		panic("sfs: %s: reclaim vnode %u not in vnode pool\n",
		      sfs->sfs_sb.sb_volname, sv->sv_ino);
	}
	vnodearray_remove(sfs->sfs_vnodes, ix);

	vnode_cleanup(&sv->sv_absvn);

	vfs_biglock_release();

	/* Release the storage for the vnode structure itself. */
	kfree(sv);

	/* Done */
	return 0;
}

/*
 * Function to load a inode into memory as a vnode, or dig up one
 * that's already resident.
 */
int
sfs_loadvnode(struct sfs_fs *sfs, uint32_t ino, int forcetype,
		 struct sfs_vnode **ret)
{
	struct vnode *v;
	struct sfs_vnode *sv;
	const struct vnode_ops *ops;
	unsigned i, num;
	int result;

	/* Look in the vnodes table */
	num = vnodearray_num(sfs->sfs_vnodes);

	/* Linear search. Is this too slow? You decide. */
	for (i=0; i<num; i++) {
		v = vnodearray_get(sfs->sfs_vnodes, i);
		sv = v->vn_data;

		/* Every inode in memory must be in an allocated block */
		if (!sfs_bused(sfs, sv->sv_ino)) {
			panic("sfs: %s: Found inode %u in unallocated block\n",
			      sfs->sfs_sb.sb_volname, sv->sv_ino);
		}

		if (sv->sv_ino==ino) {
			/* Found */

			/* forcetype is only allowed when creating objects */
			KASSERT(forcetype==SFS_TYPE_INVAL);

			VOP_INCREF(&sv->sv_absvn);
			*ret = sv;
			return 0;
		}
	}

	/* Didn't have it loaded; load it */

	sv = kmalloc(sizeof(struct sfs_vnode));
	if (sv==NULL) {
		return ENOMEM;
	}

	/* Must be in an allocated block */
	if (!sfs_bused(sfs, ino)) {
		panic("sfs: %s: Tried to load inode %u from "
		      "unallocated block\n", sfs->sfs_sb.sb_volname, ino);
	}

	/* Read the block the inode is in */
	result = sfs_readblock(sfs, ino, &sv->sv_i, sizeof(sv->sv_i));
	if (result) {
		kfree(sv);
		return result;
	}

	/* Not dirty yet */
	sv->sv_dirty = false;

	/*
	 * FORCETYPE is set if we're creating a new file, because the
	 * block on disk will have been zeroed out by sfs_balloc and
	 * thus the type recorded there will be SFS_TYPE_INVAL.
	 */
	if (forcetype != SFS_TYPE_INVAL) {
		KASSERT(sv->sv_i.sfi_type == SFS_TYPE_INVAL);
		sv->sv_i.sfi_type = forcetype;
		sv->sv_dirty = true;
	}

	/*
	 * Choose the function table based on the object type.
	 */
	switch (sv->sv_i.sfi_type) {
	    case SFS_TYPE_FILE:
		ops = &sfs_fileops;
		break;
	    case SFS_TYPE_DIR:
		ops = &sfs_dirops;
		break;
	    default:
		panic("sfs: %s: loadvnode: Invalid inode type "
		      "(inode %u, type %u)\n", sfs->sfs_sb.sb_volname,
		      ino, sv->sv_i.sfi_type);
	}

	/* Call the common vnode initializer */
	result = vnode_init(&sv->sv_absvn, ops, &sfs->sfs_absfs, sv);
	if (result) {
		kfree(sv);
		return result;
	}

	/* Set the other fields in our vnode structure */
	sv->sv_ino = ino;

	/* Add it to our table */
	result = vnodearray_add(sfs->sfs_vnodes, &sv->sv_absvn, NULL);
	if (result) {
		vnode_cleanup(&sv->sv_absvn);
		kfree(sv);
		return result;
	}

	/* Hand it back */
	*ret = sv;
	return 0;
}

/*
 * Create a new filesystem object and hand back its vnode.
 */
int
sfs_makeobj(struct sfs_fs *sfs, int type, struct sfs_vnode **ret)
{
	uint32_t ino;
	int result;

	/*
	 * First, get an inode. (Each inode is a block, and the inode
	 * number is the block number, so just get a block.)
	 */

	result = sfs_balloc(sfs, &ino);
	if (result) {
		return result;
	}

	/*
	 * Now load a vnode for it.
	 */

	result = sfs_loadvnode(sfs, ino, type, ret);
	if (result) {
		sfs_bfree(sfs, ino);
	}
	return result;
}

/*
 * Get vnode for the root of the filesystem.
 * The root vnode is always found in block 1 (SFS_ROOTDIR_INO).
 */
int
sfs_getroot(struct fs *fs, struct vnode **ret)
{
	struct sfs_fs *sfs = fs->fs_data;
	struct sfs_vnode *sv;
	int result;

	vfs_biglock_acquire();

	result = sfs_loadvnode(sfs, SFS_ROOTDIR_INO, SFS_TYPE_INVAL, &sv);
	if (result) {
		kprintf("sfs: %s: getroot: Cannot load root vnode\n",
			sfs->sfs_sb.sb_volname);
		vfs_biglock_release();
		return result;
	}

	if (sv->sv_i.sfi_type != SFS_TYPE_DIR) {
		kprintf("sfs: %s: getroot: not directory (type %u)\n",
			sfs->sfs_sb.sb_volname, sv->sv_i.sfi_type);
		vfs_biglock_release();
		return EINVAL;
	}

	vfs_biglock_release();

	*ret = &sv->sv_absvn;
	return 0;
}
