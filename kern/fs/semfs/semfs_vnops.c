/*
 * Copyright (c) 2014
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

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <stat.h>
#include <uio.h>
#include <synch.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <vfs.h>
#include <vnode.h>

#include "semfs.h"

////////////////////////////////////////////////////////////
// basic ops

static
int
semfs_eachopen(struct vnode *vn, int openflags)
{
	struct semfs_vnode *semv = vn->vn_data;

	if (semv->semv_semnum == SEMFS_ROOTDIR) {
		if ((openflags & O_ACCMODE) != O_RDONLY) {
			return EISDIR;
		}
		if (openflags & O_APPEND) {
			return EISDIR;
		}
	}

	return 0;
}

static
int
semfs_ioctl(struct vnode *vn, int op, userptr_t data)
{
	(void)vn;
	(void)op;
	(void)data;
	return EINVAL;
}

static
int
semfs_gettype(struct vnode *vn, mode_t *ret)
{
	struct semfs_vnode *semv = vn->vn_data;

	*ret = semv->semv_semnum == SEMFS_ROOTDIR ? S_IFDIR : S_IFREG;
	return 0;
}

static
bool
semfs_isseekable(struct vnode *vn)
{
	struct semfs_vnode *semv = vn->vn_data;

	if (semv->semv_semnum != SEMFS_ROOTDIR) {
		/* seeking a semaphore doesn't mean anything */
		return false;
	}
	return true;
}

static
int
semfs_fsync(struct vnode *vn)
{
	(void)vn;
	return 0;
}

////////////////////////////////////////////////////////////
// semaphore ops

/*
 * XXX fold these two together
 */

static
struct semfs_sem *
semfs_getsembynum(struct semfs *semfs, unsigned semnum)
{
	struct semfs_sem *sem;

	lock_acquire(semfs->semfs_tablelock);
	sem = semfs_semarray_get(semfs->semfs_sems, semnum);
	lock_release(semfs->semfs_tablelock);

	return sem;
}

static
struct semfs_sem *
semfs_getsem(struct semfs_vnode *semv)
{
	struct semfs *semfs = semv->semv_semfs;

	return semfs_getsembynum(semfs, semv->semv_semnum);
}

/*
 * Wakeup helper. We only need to wake up if there are sleepers, which
 * should only be the case if the old count is 0; and we only
 * potentially need to wake more than one sleeper if the new count
 * will be more than 1.
 */
static
void
semfs_wakeup(struct semfs_sem *sem, unsigned newcount)
{
	if (sem->sems_count > 0 || newcount == 0) {
		return;
	}
	if (newcount == 1) {
		cv_signal(sem->sems_cv, sem->sems_lock);
	}
	else {
		cv_broadcast(sem->sems_cv, sem->sems_lock);
	}
}

/*
 * stat() for semaphore vnodes
 */
static
int
semfs_semstat(struct vnode *vn, struct stat *buf)
{
	struct semfs_vnode *semv = vn->vn_data;
	struct semfs_sem *sem;

	sem = semfs_getsem(semv);

	bzero(buf, sizeof(*buf));

	lock_acquire(sem->sems_lock);
	buf->st_size = sem->sems_count;
	buf->st_nlink = sem->sems_linked ? 1 : 0;
	lock_release(sem->sems_lock);

	buf->st_mode = S_IFREG | 0666;
	buf->st_blocks = 0;
	buf->st_dev = 0;
	buf->st_ino = semv->semv_semnum;

	return 0;
}

/*
 * Read. This is P(); decrease the count by the amount read.
 * Don't actually bother to transfer any data.
 */
static
int
semfs_read(struct vnode *vn, struct uio *uio)
{
	struct semfs_vnode *semv = vn->vn_data;
	struct semfs_sem *sem;
	size_t consume;

	sem = semfs_getsem(semv);

	lock_acquire(sem->sems_lock);
	while (uio->uio_resid > 0) {
		if (sem->sems_count > 0) {
			consume = uio->uio_resid;
			if (consume > sem->sems_count) {
				consume = sem->sems_count;
			}
			DEBUG(DB_SEMFS, "semfs: sem%u: P, count %u -> %u\n",
			      semv->semv_semnum, sem->sems_count,
			      sem->sems_count - consume);
			sem->sems_count -= consume;
			/* don't bother advancing the uio data pointers */
			uio->uio_offset += consume;
			uio->uio_resid -= consume;
		}
		if (uio->uio_resid == 0) {
			break;
		}
		if (sem->sems_count == 0) {
			DEBUG(DB_SEMFS, "semfs: sem%u: blocking\n",
			      semv->semv_semnum);
			cv_wait(sem->sems_cv, sem->sems_lock);
		}
	}
	lock_release(sem->sems_lock);
	return 0;
}

/*
 * Write. This is V(); increase the count by the amount written.
 * Don't actually bother to transfer any data.
 */
static
int
semfs_write(struct vnode *vn, struct uio *uio)
{
	struct semfs_vnode *semv = vn->vn_data;
	struct semfs_sem *sem;
	unsigned newcount;

	sem = semfs_getsem(semv);

	lock_acquire(sem->sems_lock);
	while (uio->uio_resid > 0) {
		newcount = sem->sems_count + uio->uio_resid;
		if (newcount < sem->sems_count) {
			/* overflow */
			lock_release(sem->sems_lock);
			return EFBIG;
		}
		DEBUG(DB_SEMFS, "semfs: sem%u: V, count %u -> %u\n",
		      semv->semv_semnum, sem->sems_count, newcount);
		semfs_wakeup(sem, newcount);
		sem->sems_count = newcount;
		uio->uio_offset += uio->uio_resid;
		uio->uio_resid = 0;
	}
	lock_release(sem->sems_lock);
	return 0;
}

/*
 * Truncate. Set the count to the specified value.
 *
 * This is slightly cheesy but it allows open(..., O_TRUNC) to reset a
 * semaphore as one would expect. Also it allows creating semaphores
 * and then initializing their counts to values other than zero.
 */
static
int
semfs_truncate(struct vnode *vn, off_t len)
{
	/* We should just use UINT_MAX but we don't have it in the kernel */
	const unsigned max = (unsigned)-1;

	struct semfs_vnode *semv = vn->vn_data;
	struct semfs_sem *sem;
	unsigned newcount;

	if (len < 0) {
		return EINVAL;
	}
	if (len > (off_t)max) {
		return EFBIG;
	}
	newcount = len;

	sem = semfs_getsem(semv);

	lock_acquire(sem->sems_lock);
	semfs_wakeup(sem, newcount);
	sem->sems_count = newcount;
	lock_release(sem->sems_lock);

	return 0;
}

////////////////////////////////////////////////////////////
// directory ops

/*
 * Directory read. Note that there's only one directory (the semfs
 * root) that has all the semaphores in it.
 */
static
int
semfs_getdirentry(struct vnode *dirvn, struct uio *uio)
{
	struct semfs_vnode *dirsemv = dirvn->vn_data;
	struct semfs *semfs = dirsemv->semv_semfs;
	struct semfs_direntry *dent;
	unsigned num, pos;
	int result;

	KASSERT(uio->uio_offset >= 0);
	pos = uio->uio_offset;

	lock_acquire(semfs->semfs_dirlock);

	num = semfs_direntryarray_num(semfs->semfs_dents);
	if (pos >= num) {
		/* EOF */
		result = 0;
	}
	else {
		dent = semfs_direntryarray_get(semfs->semfs_dents, pos);
		result = uiomove(dent->semd_name, strlen(dent->semd_name),
				 uio);
	}

	lock_release(semfs->semfs_dirlock);
	return result;
}

/*
 * stat() for dirs
 */
static
int
semfs_dirstat(struct vnode *vn, struct stat *buf)
{
	struct semfs_vnode *semv = vn->vn_data;
	struct semfs *semfs = semv->semv_semfs;

	bzero(buf, sizeof(*buf));

	lock_acquire(semfs->semfs_dirlock);
	buf->st_size = semfs_direntryarray_num(semfs->semfs_dents);
	lock_release(semfs->semfs_dirlock);

	buf->st_mode = S_IFDIR | 1777;
	buf->st_nlink = 2;
	buf->st_blocks = 0;
	buf->st_dev = 0;
	buf->st_ino = SEMFS_ROOTDIR;

	return 0;
}

/*
 * Backend for getcwd. Since we don't support subdirs, it's easy; send
 * back the empty string.
 */
static
int
semfs_namefile(struct vnode *vn, struct uio *uio)
{
	(void)vn;
	(void)uio;
	return 0;
}

/*
 * Create a semaphore.
 */
static
int
semfs_creat(struct vnode *dirvn, const char *name, bool excl, mode_t mode,
	    struct vnode **resultvn)
{
	struct semfs_vnode *dirsemv = dirvn->vn_data;
	struct semfs *semfs = dirsemv->semv_semfs;
	struct semfs_direntry *dent;
	struct semfs_sem *sem;
	unsigned i, num, empty, semnum;
	int result;

	(void)mode;
	if (!strcmp(name, ".") || !strcmp(name, "..")) {
		return EEXIST;
	}

	lock_acquire(semfs->semfs_dirlock);
	num = semfs_direntryarray_num(semfs->semfs_dents);
	empty = num;
	for (i=0; i<num; i++) {
		dent = semfs_direntryarray_get(semfs->semfs_dents, i);
		if (dent == NULL) {
			if (empty == num) {
				empty = i;
			}
			continue;
		}
		if (!strcmp(dent->semd_name, name)) {
			/* found */
			if (excl) {
				lock_release(semfs->semfs_dirlock);
				return EEXIST;
			}
			result = semfs_getvnode(semfs, dent->semd_semnum,
						resultvn);
			lock_release(semfs->semfs_dirlock);
			return result;
		}
	}

	/* create it */
	sem = semfs_sem_create(name);
	if (sem == NULL) {
		result = ENOMEM;
		goto fail_unlock;
	}
	lock_acquire(semfs->semfs_tablelock);
	result = semfs_sem_insert(semfs, sem, &semnum);
	lock_release(semfs->semfs_tablelock);
	if (result) {
		goto fail_uncreate;
	}

	dent = semfs_direntry_create(name, semnum);
	if (dent == NULL) {
		goto fail_uninsert;
	}

	if (empty < num) {
		semfs_direntryarray_set(semfs->semfs_dents, empty, dent);
	}
	else {
		result = semfs_direntryarray_add(semfs->semfs_dents, dent,
						 &empty);
		if (result) {
			goto fail_undent;
		}
	}

	result = semfs_getvnode(semfs, semnum, resultvn);
	if (result) {
		goto fail_undir;
	}

	sem->sems_linked = true;
	lock_release(semfs->semfs_dirlock);
	return 0;

 fail_undir:
	semfs_direntryarray_set(semfs->semfs_dents, empty, NULL);
 fail_undent:
	semfs_direntry_destroy(dent);
 fail_uninsert:
	lock_acquire(semfs->semfs_tablelock);
	semfs_semarray_set(semfs->semfs_sems, semnum, NULL);
	lock_release(semfs->semfs_tablelock);
 fail_uncreate:
	semfs_sem_destroy(sem);
 fail_unlock:
	lock_release(semfs->semfs_dirlock);
	return result;
}

/*
 * Unlink a semaphore. As with other files, it may not actually
 * go away if it's currently open.
 */
static
int
semfs_remove(struct vnode *dirvn, const char *name)
{
	struct semfs_vnode *dirsemv = dirvn->vn_data;
	struct semfs *semfs = dirsemv->semv_semfs;
	struct semfs_direntry *dent;
	struct semfs_sem *sem;
	unsigned i, num;
	int result;

	if (!strcmp(name, ".") || !strcmp(name, "..")) {
		return EINVAL;
	}

	lock_acquire(semfs->semfs_dirlock);
	num = semfs_direntryarray_num(semfs->semfs_dents);
	for (i=0; i<num; i++) {
		dent = semfs_direntryarray_get(semfs->semfs_dents, i);
		if (dent == NULL) {
			continue;
		}
		if (!strcmp(name, dent->semd_name)) {
			/* found */
			sem = semfs_getsembynum(semfs, dent->semd_semnum);
			lock_acquire(sem->sems_lock);
			KASSERT(sem->sems_linked);
			sem->sems_linked = false;
			if (sem->sems_hasvnode == false) {
				lock_acquire(semfs->semfs_tablelock);
				semfs_semarray_set(semfs->semfs_sems,
						   dent->semd_semnum, NULL);
				lock_release(semfs->semfs_tablelock);
				lock_release(sem->sems_lock);
				semfs_sem_destroy(sem);
			}
			else {
				lock_release(sem->sems_lock);
			}
			semfs_direntryarray_set(semfs->semfs_dents, i, NULL);
			semfs_direntry_destroy(dent);
			result = 0;
			goto out;
		}
	}
	result = ENOENT;
 out:
	lock_release(semfs->semfs_dirlock);
	return result;
}

/*
 * Lookup: get a semaphore by name.
 */
static
int
semfs_lookup(struct vnode *dirvn, char *path, struct vnode **resultvn)
{
	struct semfs_vnode *dirsemv = dirvn->vn_data;
	struct semfs *semfs = dirsemv->semv_semfs;
	struct semfs_direntry *dent;
	unsigned i, num;
	int result;

	if (!strcmp(path, ".") || !strcmp(path, "..")) {
		VOP_INCREF(dirvn);
		*resultvn = dirvn;
		return 0;
	}

	lock_acquire(semfs->semfs_dirlock);
	num = semfs_direntryarray_num(semfs->semfs_dents);
	for (i=0; i<num; i++) {
		dent = semfs_direntryarray_get(semfs->semfs_dents, i);
		if (dent == NULL) {
			continue;
		}
		if (!strcmp(path, dent->semd_name)) {
			result = semfs_getvnode(semfs, dent->semd_semnum,
						resultvn);
			lock_release(semfs->semfs_dirlock);
			return result;
		}
	}
	lock_release(semfs->semfs_dirlock);
	return ENOENT;
}

/*
 * Lookparent: because we don't have subdirs, just return the root
 * dir and copy the name.
 */
static
int
semfs_lookparent(struct vnode *dirvn, char *path,
		 struct vnode **resultdirvn, char *namebuf, size_t bufmax)
{
        if (strlen(path)+1 > bufmax) {
                return ENAMETOOLONG;
        }
        strcpy(namebuf, path);

        VOP_INCREF(dirvn);
        *resultdirvn = dirvn;
	return 0;
}

////////////////////////////////////////////////////////////
// vnode lifecycle operations

/*
 * Destructor for semfs_vnode.
 */
static
void
semfs_vnode_destroy(struct semfs_vnode *semv)
{
	vnode_cleanup(&semv->semv_absvn);
	kfree(semv);
}

/*
 * Reclaim - drop a vnode that's no longer in use.
 */
static
int
semfs_reclaim(struct vnode *vn)
{
	struct semfs_vnode *semv = vn->vn_data;
	struct semfs *semfs = semv->semv_semfs;
	struct vnode *vn2;
	struct semfs_sem *sem;
	unsigned i, num;

	lock_acquire(semfs->semfs_tablelock);

	/* vnode refcount is protected by the vnode's ->vn_countlock */
	spinlock_acquire(&vn->vn_countlock);
	if (vn->vn_refcount > 1) {
		/* consume the reference VOP_DECREF passed us */
		vn->vn_refcount--;

		spinlock_release(&vn->vn_countlock);
		lock_release(semfs->semfs_tablelock);
		return EBUSY;
	}

	spinlock_release(&vn->vn_countlock);

	/* remove from the table */
	num = vnodearray_num(semfs->semfs_vnodes);
	for (i=0; i<num; i++) {
		vn2 = vnodearray_get(semfs->semfs_vnodes, i);
		if (vn2 == vn) {
			vnodearray_remove(semfs->semfs_vnodes, i);
			break;
		}
	}

	if (semv->semv_semnum != SEMFS_ROOTDIR) {
		sem = semfs_semarray_get(semfs->semfs_sems, semv->semv_semnum);
		KASSERT(sem->sems_hasvnode);
		sem->sems_hasvnode = false;
		if (sem->sems_linked == false) {
			semfs_semarray_set(semfs->semfs_sems,
					   semv->semv_semnum, NULL);
			semfs_sem_destroy(sem);
		}
	}

	/* done with the table */
	lock_release(semfs->semfs_tablelock);

	/* destroy it */
	semfs_vnode_destroy(semv);
	return 0;
}

/*
 * Vnode ops table for dirs.
 */
static const struct vnode_ops semfs_dirops = {
	.vop_magic = VOP_MAGIC,

	.vop_eachopen = semfs_eachopen,
	.vop_reclaim = semfs_reclaim,

	.vop_read = vopfail_uio_isdir,
	.vop_readlink = vopfail_uio_isdir,
	.vop_getdirentry = semfs_getdirentry,
	.vop_write = vopfail_uio_isdir,
	.vop_ioctl = semfs_ioctl,
	.vop_stat = semfs_dirstat,
	.vop_gettype = semfs_gettype,
	.vop_isseekable = semfs_isseekable,
	.vop_fsync = semfs_fsync,
	.vop_mmap = vopfail_mmap_isdir,
	.vop_truncate = vopfail_truncate_isdir,
	.vop_namefile = semfs_namefile,

	.vop_creat = semfs_creat,
	.vop_symlink = vopfail_symlink_nosys,
	.vop_mkdir = vopfail_mkdir_nosys,
	.vop_link = vopfail_link_nosys,
	.vop_remove = semfs_remove,
	.vop_rmdir = vopfail_string_nosys,
	.vop_rename = vopfail_rename_nosys,
	.vop_lookup = semfs_lookup,
	.vop_lookparent = semfs_lookparent,
};

/*
 * Vnode ops table for semaphores (files).
 */
static const struct vnode_ops semfs_semops = {
	.vop_magic = VOP_MAGIC,

	.vop_eachopen = semfs_eachopen,
	.vop_reclaim = semfs_reclaim,

	.vop_read = semfs_read,
	.vop_readlink = vopfail_uio_inval,
	.vop_getdirentry = vopfail_uio_notdir,
	.vop_write = semfs_write,
	.vop_ioctl = semfs_ioctl,
	.vop_stat = semfs_semstat,
	.vop_gettype = semfs_gettype,
	.vop_isseekable = semfs_isseekable,
	.vop_fsync = semfs_fsync,
	.vop_mmap = vopfail_mmap_perm,
	.vop_truncate = semfs_truncate,
	.vop_namefile = vopfail_uio_notdir,

	.vop_creat = vopfail_creat_notdir,
	.vop_symlink = vopfail_symlink_notdir,
	.vop_mkdir = vopfail_mkdir_notdir,
	.vop_link = vopfail_link_notdir,
	.vop_remove = vopfail_string_notdir,
	.vop_rmdir = vopfail_string_notdir,
	.vop_rename = vopfail_rename_notdir,
	.vop_lookup = vopfail_lookup_notdir,
	.vop_lookparent = vopfail_lookparent_notdir,
};

/*
 * Constructor for semfs vnodes.
 */
static
struct semfs_vnode *
semfs_vnode_create(struct semfs *semfs, unsigned semnum)
{
	const struct vnode_ops *optable;
	struct semfs_vnode *semv;
	int result;

	if (semnum == SEMFS_ROOTDIR) {
		optable = &semfs_dirops;
	}
	else {
		optable = &semfs_semops;
	}

	semv = kmalloc(sizeof(*semv));
	if (semv == NULL) {
		return NULL;
	}

	semv->semv_semfs = semfs;
	semv->semv_semnum = semnum;

	result = vnode_init(&semv->semv_absvn, optable,
			    &semfs->semfs_absfs, semv);
	/* vnode_init doesn't actually fail */
	KASSERT(result == 0);

	return semv;
}

/*
 * Look up the vnode for a semaphore by number; if it doesn't exist,
 * create it.
 */
int
semfs_getvnode(struct semfs *semfs, unsigned semnum, struct vnode **ret)
{
	struct vnode *vn;
	struct semfs_vnode *semv;
	struct semfs_sem *sem;
	unsigned i, num;
	int result;

	/* Lock the vnode table */
	lock_acquire(semfs->semfs_tablelock);

	/* Look for it */
	num = vnodearray_num(semfs->semfs_vnodes);
	for (i=0; i<num; i++) {
		vn = vnodearray_get(semfs->semfs_vnodes, i);
		semv = vn->vn_data;
		if (semv->semv_semnum == semnum) {
			VOP_INCREF(vn);
			lock_release(semfs->semfs_tablelock);
			*ret = vn;
			return 0;
		}
	}

	/* Make it */
	semv = semfs_vnode_create(semfs, semnum);
	if (semv == NULL) {
		lock_release(semfs->semfs_tablelock);
		return ENOMEM;
	}
	result = vnodearray_add(semfs->semfs_vnodes, &semv->semv_absvn, NULL);
	if (result) {
		semfs_vnode_destroy(semv);
		lock_release(semfs->semfs_tablelock);
		return ENOMEM;
	}
	if (semnum != SEMFS_ROOTDIR) {
		sem = semfs_semarray_get(semfs->semfs_sems, semnum);
		KASSERT(sem != NULL);
		KASSERT(sem->sems_hasvnode == false);
		sem->sems_hasvnode = true;
	}
	lock_release(semfs->semfs_tablelock);

	*ret = &semv->semv_absvn;
	return 0;
}
