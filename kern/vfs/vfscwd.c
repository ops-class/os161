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

/*
 * VFS operations involving the current directory.
 */

#include <types.h>
#include <kern/errno.h>
#include <stat.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <vfs.h>
#include <fs.h>
#include <vnode.h>

/*
 * Get current directory as a vnode.
 */
int
vfs_getcurdir(struct vnode **ret)
{
	int rv = 0;

	spinlock_acquire(&curproc->p_lock);
	if (curproc->p_cwd!=NULL) {
		VOP_INCREF(curproc->p_cwd);
		*ret = curproc->p_cwd;
	}
	else {
		rv = ENOENT;
	}
	spinlock_release(&curproc->p_lock);

	return rv;
}

/*
 * Set current directory as a vnode.
 * The passed vnode must in fact be a directory.
 */
int
vfs_setcurdir(struct vnode *dir)
{
	struct vnode *old;
	mode_t vtype;
	int result;

	result = VOP_GETTYPE(dir, &vtype);
	if (result) {
		return result;
	}
	if (vtype != S_IFDIR) {
		return ENOTDIR;
	}

	VOP_INCREF(dir);

	spinlock_acquire(&curproc->p_lock);
	old = curproc->p_cwd;
	curproc->p_cwd = dir;
	spinlock_release(&curproc->p_lock);

	if (old!=NULL) {
		VOP_DECREF(old);
	}

	return 0;
}

/*
 * Set current directory to "none".
 */
int
vfs_clearcurdir(void)
{
	struct vnode *old;

	spinlock_acquire(&curproc->p_lock);
	old = curproc->p_cwd;
	curproc->p_cwd = NULL;
	spinlock_release(&curproc->p_lock);

	if (old!=NULL) {
		VOP_DECREF(old);
	}

	return 0;
}

/*
 * Set current directory, as a pathname. Use vfs_lookup to translate
 * it to a vnode.
 */
int
vfs_chdir(char *path)
{
	struct vnode *vn;
	int result;

	result = vfs_lookup(path, &vn);
	if (result) {
		return result;
	}
	result = vfs_setcurdir(vn);
	VOP_DECREF(vn);
	return result;
}

/*
 * Get current directory, as a pathname.
 * Use VOP_NAMEFILE to get the pathname and FSOP_GETVOLNAME to get the
 * volume name.
 */
int
vfs_getcwd(struct uio *uio)
{
	struct vnode *cwd;
	int result;
	const char *name;
	char colon=':';

	KASSERT(uio->uio_rw==UIO_READ);

	result = vfs_getcurdir(&cwd);
	if (result) {
		return result;
	}

	/* The current dir must be a directory, and thus it is not a device. */
	KASSERT(cwd->vn_fs != NULL);

	name = FSOP_GETVOLNAME(cwd->vn_fs);
	if (name==NULL) {
		vfs_biglock_acquire();
		name = vfs_getdevname(cwd->vn_fs);
		vfs_biglock_release();
	}
	KASSERT(name != NULL);

	result = uiomove((char *)name, strlen(name), uio);
	if (result) {
		goto out;
	}
	result = uiomove(&colon, 1, uio);
	if (result) {
		goto out;
	}

	result = VOP_NAMEFILE(cwd, uio);

 out:

	VOP_DECREF(cwd);
	return result;
}
