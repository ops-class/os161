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
 * Vnode operations for VFS devices.
 *
 * These hand off to the functions in the VFS device structure (see dev.h)
 * but take care of a bunch of common tasks in a uniform fashion.
 */
#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <stat.h>
#include <lib.h>
#include <uio.h>
#include <synch.h>
#include <vnode.h>
#include <device.h>

/*
 * Called for each open().
 *
 * We reject O_APPEND.
 */
static
int
dev_eachopen(struct vnode *v, int flags)
{
	struct device *d = v->vn_data;

	if (flags & (O_CREAT | O_TRUNC | O_EXCL | O_APPEND)) {
		return EINVAL;
	}

	return DEVOP_EACHOPEN(d, flags);
}

/*
 * Called when the vnode refcount reaches zero.
 * Do nothing; devices are permanent.
 */
static
int
dev_reclaim(struct vnode *v)
{
	(void)v;
	/* nothing - device continues to exist even when not in use */
	return 0;
}

/*
 * Check a seek position.
 *
 * For block devices, require block alignment.
 * For character devices, we should prohibit seeking entirely, but
 * for the moment we need to accept any position. (XXX)
 */
static
int
dev_tryseek(struct device *d, off_t pos)
{
	if (d->d_blocks > 0) {
		if ((pos % d->d_blocksize)!=0) {
			/* not block-aligned */
			return EINVAL;
		}
		if (pos / d->d_blocksize >= d->d_blocks) {
			/* off the end */
			return EINVAL;
		}
	}
	else {
		//return ESPIPE;
	}
	return 0;
}

/*
 * Called for read. Hand off to DEVOP_IO.
 */
static
int
dev_read(struct vnode *v, struct uio *uio)
{
	struct device *d = v->vn_data;
	int result;

	result = dev_tryseek(d, uio->uio_offset);
	if (result) {
		return result;
	}

	KASSERT(uio->uio_rw == UIO_READ);
	return DEVOP_IO(d, uio);
}

/*
 * Called for write. Hand off to DEVOP_IO.
 */
static
int
dev_write(struct vnode *v, struct uio *uio)
{
	struct device *d = v->vn_data;
	int result;

	result = dev_tryseek(d, uio->uio_offset);
	if (result) {
		return result;
	}

	KASSERT(uio->uio_rw == UIO_WRITE);
	return DEVOP_IO(d, uio);
}

/*
 * Called for ioctl(). Just pass through.
 */
static
int
dev_ioctl(struct vnode *v, int op, userptr_t data)
{
	struct device *d = v->vn_data;
	return DEVOP_IOCTL(d, op, data);
}

/*
 * Called for stat().
 * Set the type and the size (block devices only).
 * The link count for a device is always 1.
 */
static
int
dev_stat(struct vnode *v, struct stat *statbuf)
{
	struct device *d = v->vn_data;
	int result;

	bzero(statbuf, sizeof(struct stat));

	if (d->d_blocks > 0) {
		statbuf->st_size = d->d_blocks * d->d_blocksize;
		statbuf->st_blksize = d->d_blocksize;
	}
	else {
		statbuf->st_size = 0;
	}

	result = VOP_GETTYPE(v, &statbuf->st_mode);
	if (result) {
		return result;
	}
	/* Make up some plausible default permissions. */
	statbuf->st_mode |= 0600;

	statbuf->st_nlink = 1;
	statbuf->st_blocks = d->d_blocks;

	/* The device number this device sits on (in OS/161, it doesn't) */
	statbuf->st_dev = 0;

	/* The device number this device *is* */
	statbuf->st_rdev = d->d_devnumber;

	return 0;
}

/*
 * Return the type. A device is a "block device" if it has a known
 * length. A device that generates data in a stream is a "character
 * device".
 */
static
int
dev_gettype(struct vnode *v, mode_t *ret)
{
	struct device *d = v->vn_data;
	if (d->d_blocks > 0) {
		*ret = S_IFBLK;
	}
	else {
		*ret = S_IFCHR;
	}
	return 0;
}

/*
 * Check if seeking is allowed.
 */
static
bool
dev_isseekable(struct vnode *v)
{
	struct device *d = v->vn_data;

	if (d->d_blocks == 0) {
		return false;
	}
	return true;
}

/*
 * For fsync() - meaningless, do nothing.
 */
static
int
null_fsync(struct vnode *v)
{
	(void)v;
	return 0;
}

/*
 * For mmap. If you want this to do anything, you have to write it
 * yourself. Some devices may not make sense to map. Others do.
 */
static
int
dev_mmap(struct vnode *v  /* add stuff as needed */)
{
	(void)v;
	return ENOSYS;
}

/*
 * For ftruncate().
 */
static
int
dev_truncate(struct vnode *v, off_t len)
{
	struct device *d = v->vn_data;

	/*
	 * Allow truncating to the object's own size, if it has one.
	 */
	if (d->d_blocks > 0 && (off_t)(d->d_blocks*d->d_blocksize) == len) {
		return 0;
	}

	return EINVAL;
}

/*
 * For namefile (which implements "pwd")
 *
 * This should never be reached, as it's not possible to chdir to a
 * device vnode.
 */
static
int
dev_namefile(struct vnode *v, struct uio *uio)
{
	/*
	 * The name of a device is always just "device:". The VFS
	 * layer puts in the device name for us, so we don't need to
	 * do anything further.
	 */

	(void)v;
	(void)uio;

	return 0;
}

/*
 * Name lookup.
 *
 * One interesting feature of device:name pathname syntax is that you
 * can implement pathnames on arbitrary devices. For instance, if you
 * had a graphics device that supported multiple resolutions (which we
 * don't), you might arrange things so that you could open it with
 * pathnames like "video:800x600/24bpp" in order to select the operating
 * mode.
 *
 * However, we have no support for this in the base system.
 */
static
int
dev_lookup(struct vnode *dir,
	   char *pathname, struct vnode **result)
{
	/*
	 * If the path was "device:", we get "". For that, return self.
	 * Anything else is an error.
	 * Increment the ref count of the vnode before returning it.
	 */
	if (strlen(pathname)>0) {
		return ENOENT;
	}
	VOP_INCREF(dir);
	*result = dir;
	return 0;
}

/*
 * Function table for device vnodes.
 */
static const struct vnode_ops dev_vnode_ops = {
	.vop_magic = VOP_MAGIC,

	.vop_eachopen = dev_eachopen,
	.vop_reclaim = dev_reclaim,
	.vop_read = dev_read,
	.vop_readlink = vopfail_uio_inval,
	.vop_getdirentry = vopfail_uio_notdir,
	.vop_write = dev_write,
	.vop_ioctl = dev_ioctl,
	.vop_stat = dev_stat,
	.vop_gettype = dev_gettype,
	.vop_isseekable = dev_isseekable,
	.vop_fsync = null_fsync,
	.vop_mmap = dev_mmap,
	.vop_truncate = dev_truncate,
	.vop_namefile = dev_namefile,
	.vop_creat = vopfail_creat_notdir,
	.vop_symlink = vopfail_symlink_notdir,
	.vop_mkdir = vopfail_mkdir_notdir,
	.vop_link = vopfail_link_notdir,
	.vop_remove = vopfail_string_notdir,
	.vop_rmdir = vopfail_string_notdir,
	.vop_rename = vopfail_rename_notdir,
	.vop_lookup = dev_lookup,
	.vop_lookparent = vopfail_lookparent_notdir,
};

/*
 * Function to create a vnode for a VFS device.
 */
struct vnode *
dev_create_vnode(struct device *dev)
{
	int result;
	struct vnode *v;

	v = kmalloc(sizeof(struct vnode));
	if (v==NULL) {
		return NULL;
	}

	result = vnode_init(v, &dev_vnode_ops, NULL, dev);
	if (result != 0) {
		panic("While creating vnode for device: vnode_init: %s\n",
		      strerror(result));
	}

	return v;
}

/*
 * Undo dev_create_vnode.
 *
 * Note: this is only used in failure paths; we don't support
 * hotpluggable devices, so once a device is attached it's permanent.
 */
void
dev_uncreate_vnode(struct vnode *vn)
{
	KASSERT(vn->vn_ops == &dev_vnode_ops);
	vnode_cleanup(vn);
	kfree(vn);
}
