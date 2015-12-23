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
#include <synch.h>
#include <vfs.h>
#include <fs.h>
#include <vnode.h>

#include "semfs.h"

////////////////////////////////////////////////////////////
// fs-level operations

/*
 * Sync doesn't need to do anything.
 */
static
int
semfs_sync(struct fs *fs)
{
	(void)fs;
	return 0;
}

/*
 * We have only one volume name and it's hardwired.
 */
static
const char *
semfs_getvolname(struct fs *fs)
{
	(void)fs;
	return "sem";
}

/*
 * Get the root directory vnode.
 */
static
int
semfs_getroot(struct fs *fs, struct vnode **ret)
{
	struct semfs *semfs = fs->fs_data;
	struct vnode *vn;
	int result;

	result = semfs_getvnode(semfs, SEMFS_ROOTDIR, &vn);
	if (result) {
		kprintf("semfs: couldn't load root vnode: %s\n",
			strerror(result));
		return result;
	}
	*ret = vn;
	return 0;
}

////////////////////////////////////////////////////////////
// mount and unmount logic


/*
 * Destructor for struct semfs.
 */
static
void
semfs_destroy(struct semfs *semfs)
{
	struct semfs_sem *sem;
	struct semfs_direntry *dent;
	unsigned i, num;

	num = semfs_semarray_num(semfs->semfs_sems);
	for (i=0; i<num; i++) {
		sem = semfs_semarray_get(semfs->semfs_sems, i);
		semfs_sem_destroy(sem);
	}
	semfs_semarray_setsize(semfs->semfs_sems, 0);

	num = semfs_direntryarray_num(semfs->semfs_dents);
	for (i=0; i<num; i++) {
		dent = semfs_direntryarray_get(semfs->semfs_dents, i);
		semfs_direntry_destroy(dent);
	}
	semfs_direntryarray_setsize(semfs->semfs_dents, 0);

	semfs_direntryarray_destroy(semfs->semfs_dents);
	lock_destroy(semfs->semfs_dirlock);
	semfs_semarray_destroy(semfs->semfs_sems);
	vnodearray_destroy(semfs->semfs_vnodes);
	lock_destroy(semfs->semfs_tablelock);
	kfree(semfs);
}

/*
 * Unmount routine. XXX: Since semfs is attached at boot and can't be
 * remounted, maybe unmounting it shouldn't be allowed.
 */
static
int
semfs_unmount(struct fs *fs)
{
	struct semfs *semfs = fs->fs_data;

	lock_acquire(semfs->semfs_tablelock);
	if (vnodearray_num(semfs->semfs_vnodes) > 0) {
		lock_release(semfs->semfs_tablelock);
		return EBUSY;
	}

	lock_release(semfs->semfs_tablelock);
	semfs_destroy(semfs);

	return 0;
}

/*
 * Operations table.
 */
static const struct fs_ops semfs_fsops = {
	.fsop_sync = semfs_sync,
	.fsop_getvolname = semfs_getvolname,
	.fsop_getroot = semfs_getroot,
	.fsop_unmount = semfs_unmount,
};

/*
 * Constructor for struct semfs.
 */
static
struct semfs *
semfs_create(void)
{
	struct semfs *semfs;

	semfs = kmalloc(sizeof(*semfs));
	if (semfs == NULL) {
		goto fail_total;
	}

	semfs->semfs_tablelock = lock_create("semfs_table");
	if (semfs->semfs_tablelock == NULL) {
		goto fail_semfs;
	}
	semfs->semfs_vnodes = vnodearray_create();
	if (semfs->semfs_vnodes == NULL) {
		goto fail_tablelock;
	}
	semfs->semfs_sems = semfs_semarray_create();
	if (semfs->semfs_sems == NULL) {
		goto fail_vnodes;
	}

	semfs->semfs_dirlock = lock_create("semfs_dir");
	if (semfs->semfs_dirlock == NULL) {
		goto fail_sems;
	}
	semfs->semfs_dents = semfs_direntryarray_create();
	if (semfs->semfs_dents == NULL) {
		goto fail_dirlock;
	}

	semfs->semfs_absfs.fs_data = semfs;
	semfs->semfs_absfs.fs_ops = &semfs_fsops;
	return semfs;

 fail_dirlock:
	lock_destroy(semfs->semfs_dirlock);
 fail_sems:
	semfs_semarray_destroy(semfs->semfs_sems);
 fail_vnodes:
	vnodearray_destroy(semfs->semfs_vnodes);
 fail_tablelock:
	lock_destroy(semfs->semfs_tablelock);
 fail_semfs:
	kfree(semfs);
 fail_total:
	return NULL;
}

/*
 * Create the semfs. There is only one semfs and it's attached as
 * "sem:" during bootup.
 */
void
semfs_bootstrap(void)
{
	struct semfs *semfs;
	int result;

	semfs = semfs_create();
	if (semfs == NULL) {
		panic("Out of memory creating semfs\n");
	}
	result = vfs_addfs("sem", &semfs->semfs_absfs);
	if (result) {
		panic("Attaching semfs: %s\n", strerror(result));
	}
}
