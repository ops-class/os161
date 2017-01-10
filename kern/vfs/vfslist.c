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
 * VFS operations that involve the list of VFS (named) devices
 * (the "dev" in "dev:path" syntax).
 */

#define VFSINLINE

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <array.h>
#include <synch.h>
#include <vfs.h>
#include <fs.h>
#include <vnode.h>
#include <device.h>

/*
 * Structure for a single named device.
 *
 * kd_name    - Name of device (eg, "lhd0"). Should always be set to
 *              a valid string.
 *
 * kd_rawname - Name of raw device (eg, "lhd0raw"). Is non-NULL if and
 *              only if this device can have a filesystem mounted on
 *              it.
 *
 * kd_device  - Device object this name refers to. May be NULL if kd_fs
 *              is hardwired.
 *
 * kd_fs      - Filesystem object mounted on, or associated with, this
 *              device. NULL if there is no filesystem.
 *
 * A filesystem can be associated with a device without having been
 * mounted if the device was created that way. In this case,
 * kd_rawname is NULL (prohibiting mount/unmount), and, as there is
 * then no way to access kd_device, it will be NULL as well. This is
 * intended for devices that are inherently filesystems, like emu0.
 *
 * Referencing kd_name, or the filesystem volume name, on a device
 * with a filesystem mounted returns the root of the filesystem.
 * Referencing kd_name on a mountable device with no filesystem
 * returns ENXIO. Referencing kd_name on a device that is not
 * mountable and has no filesystem, or kd_rawname on a mountable
 * device, returns the device itself.
 */

struct knowndev {
	char *kd_name;
	char *kd_rawname;
	struct device *kd_device;
	struct vnode *kd_vnode;
	struct fs *kd_fs;
};

/* A placeholder for kd_fs for devices used as swap */
#define SWAP_FS	((struct fs *)-1)

DECLARRAY(knowndev, static __UNUSED inline);
DEFARRAY(knowndev, static __UNUSED inline);

static struct knowndevarray *knowndevs;

/* The big lock for all FS ops. Remove for filesystem assignment. */
static struct lock *vfs_biglock;
static unsigned vfs_biglock_depth;


/*
 * Setup function
 */
void
vfs_bootstrap(void)
{
	knowndevs = knowndevarray_create();
	if (knowndevs==NULL) {
		panic("vfs: Could not create knowndevs array\n");
	}

	vfs_biglock = lock_create("vfs_biglock");
	if (vfs_biglock==NULL) {
		panic("vfs: Could not create vfs big lock\n");
	}
	vfs_biglock_depth = 0;

	devnull_create();
	semfs_bootstrap();
}

/*
 * Operations on vfs_biglock. We make it recursive to avoid having to
 * think about where we do and don't already hold it. This is an
 * undesirable hack that's frequently necessary when a lock covers too
 * much material. Your solution scheme for FS and VFS locking should
 * not require recursive locks.
 */
void
vfs_biglock_acquire(void)
{
	if (!lock_do_i_hold(vfs_biglock)) {
		lock_acquire(vfs_biglock);
	}
	vfs_biglock_depth++;
}

void
vfs_biglock_release(void)
{
	KASSERT(lock_do_i_hold(vfs_biglock));
	KASSERT(vfs_biglock_depth > 0);
	vfs_biglock_depth--;
	if (vfs_biglock_depth == 0) {
		lock_release(vfs_biglock);
	}
}

bool
vfs_biglock_do_i_hold(void)
{
	return lock_do_i_hold(vfs_biglock);
}

/*
 * Global sync function - call FSOP_SYNC on all devices.
 */
int
vfs_sync(void)
{
	struct knowndev *dev;
	unsigned i, num;

	vfs_biglock_acquire();

	num = knowndevarray_num(knowndevs);
	for (i=0; i<num; i++) {
		dev = knowndevarray_get(knowndevs, i);
		if (dev->kd_fs != NULL && dev->kd_fs != SWAP_FS) {
			/*result =*/ FSOP_SYNC(dev->kd_fs);
		}
	}

	vfs_biglock_release();

	return 0;
}

/*
 * Given a device name (lhd0, emu0, somevolname, null, etc.), hand
 * back an appropriate vnode.
 */
int
vfs_getroot(const char *devname, struct vnode **ret)
{
	struct knowndev *kd;
	unsigned i, num;

	KASSERT(vfs_biglock_do_i_hold());

	num = knowndevarray_num(knowndevs);
	for (i=0; i<num; i++) {
		kd = knowndevarray_get(knowndevs, i);

		/*
		 * If this device has a mounted filesystem, and
		 * DEVNAME names either the filesystem or the device,
		 * return the root of the filesystem.
		 *
		 * If it has no mounted filesystem, it's mountable,
		 * and DEVNAME names the device, return ENXIO.
		 */

		if (kd->kd_fs != NULL && kd->kd_fs != SWAP_FS) {
			const char *volname;
			volname = FSOP_GETVOLNAME(kd->kd_fs);

			if (!strcmp(kd->kd_name, devname) ||
			    (volname!=NULL && !strcmp(volname, devname))) {
				return FSOP_GETROOT(kd->kd_fs, ret);
			}
		}
		else {
			if (kd->kd_rawname!=NULL &&
			    !strcmp(kd->kd_name, devname)) {
				return ENXIO;
			}
		}

		/*
		 * If DEVNAME names the device, and we get here, it
		 * must have no fs and not be mountable. In this case,
		 * we return the device itself.
		 */
		if (!strcmp(kd->kd_name, devname)) {
			KASSERT(kd->kd_fs==NULL);
			KASSERT(kd->kd_rawname==NULL);
			KASSERT(kd->kd_device != NULL);
			VOP_INCREF(kd->kd_vnode);
			*ret = kd->kd_vnode;
			return 0;
		}

		/*
		 * If the device has a rawname and DEVNAME names that,
		 * return the device itself.
		 */
		if (kd->kd_rawname!=NULL && !strcmp(kd->kd_rawname, devname)) {
			KASSERT(kd->kd_device != NULL);
			VOP_INCREF(kd->kd_vnode);
			*ret = kd->kd_vnode;
			return 0;
		}

		/*
		 * If none of the above tests matched, we didn't name
		 * any of the names of this device, so go on to the
		 * next one.
		 */
	}

	/*
	 * If we got here, the device specified by devname doesn't exist.
	 */

	return ENODEV;
}

/*
 * Given a filesystem, hand back the name of the device it's mounted on.
 */
const char *
vfs_getdevname(struct fs *fs)
{
	struct knowndev *kd;
	unsigned i, num;

	KASSERT(fs != NULL);

	KASSERT(vfs_biglock_do_i_hold());

	num = knowndevarray_num(knowndevs);
	for (i=0; i<num; i++) {
		kd = knowndevarray_get(knowndevs, i);

		if (kd->kd_fs == fs) {
			/*
			 * This is not a race condition: as long as the
			 * guy calling us holds a reference to the fs,
			 * the fs cannot go away, and the device can't
			 * go away until the fs goes away.
			 */
			return kd->kd_name;
		}
	}

	return NULL;
}

/*
 * Assemble the name for a raw device from the name for the regular device.
 */
static
char *
mkrawname(const char *name)
{
	char *s = kmalloc(strlen(name)+3+1);
	if (!s) {
		return NULL;
	}
	strcpy(s, name);
	strcat(s, "raw");
	return s;
}


/*
 * Check if the two strings passed in are the same, if they're both
 * not NULL (the latter part being significant).
 */
static
inline
int
samestring(const char *a, const char *b)
{
	if (a==NULL || b==NULL) {
		return 0;
	}
	return !strcmp(a, b);
}

/*
 * Check if the first string passed is the same as any of the three others,
 * if they're not NULL.
 */
static
inline
int
samestring3(const char *a, const char *b, const char *c, const char *d)
{
	return samestring(a,b) || samestring(a,c) || samestring(a,d);
}

/*
 * Check if any of the three names passed in already exists as a device
 * name.
 */

static
int
badnames(const char *n1, const char *n2, const char *n3)
{
	const char *volname;
	unsigned i, num;
	struct knowndev *kd;

	KASSERT(vfs_biglock_do_i_hold());

	num = knowndevarray_num(knowndevs);
	for (i=0; i<num; i++) {
		kd = knowndevarray_get(knowndevs, i);

		if (kd->kd_fs != NULL && kd->kd_fs != SWAP_FS) {
			volname = FSOP_GETVOLNAME(kd->kd_fs);
			if (samestring3(volname, n1, n2, n3)) {
				return 1;
			}
		}

		if (samestring3(kd->kd_rawname, n1, n2, n3) ||
		    samestring3(kd->kd_name, n1, n2, n3)) {
			return 1;
		}
	}

	return 0;
}

/*
 * Add a new device to the VFS layer's device table.
 *
 * If "mountable" is set, the device will be treated as one that expects
 * to have a filesystem mounted on it, and a raw device will be created
 * for direct access.
 */
static
int
vfs_doadd(const char *dname, int mountable, struct device *dev, struct fs *fs)
{
	char *name=NULL, *rawname=NULL;
	struct knowndev *kd=NULL;
	struct vnode *vnode=NULL;
	const char *volname=NULL;
	unsigned index;
	int result;

	vfs_biglock_acquire();

	name = kstrdup(dname);
	if (name==NULL) {
		result = ENOMEM;
		goto fail;
	}
	if (mountable) {
		rawname = mkrawname(name);
		if (rawname==NULL) {
			result = ENOMEM;
			goto fail;
		}
	}

	vnode = dev_create_vnode(dev);
	if (vnode==NULL) {
		result = ENOMEM;
		goto fail;
	}

	kd = kmalloc(sizeof(struct knowndev));
	if (kd==NULL) {
		result = ENOMEM;
		goto fail;
	}

	kd->kd_name = name;
	kd->kd_rawname = rawname;
	kd->kd_device = dev;
	kd->kd_vnode = vnode;
	kd->kd_fs = fs;

	if (fs!=NULL) {
		volname = FSOP_GETVOLNAME(fs);
	}

	if (badnames(name, rawname, volname)) {
		result = EEXIST;
		goto fail;
	}

	result = knowndevarray_add(knowndevs, kd, &index);
	if (result) {
		goto fail;
	}

	if (dev != NULL) {
		/* use index+1 as the device number, so 0 is reserved */
		dev->d_devnumber = index+1;
	}

	vfs_biglock_release();
	return 0;

 fail:
	if (name) {
		kfree(name);
	}
	if (rawname) {
		kfree(rawname);
	}
	if (vnode) {
		dev_uncreate_vnode(vnode);
	}
	if (kd) {
		kfree(kd);
	}

	vfs_biglock_release();
	return result;
}

/*
 * Add a new device, by name. See above for the description of
 * mountable.
 */
int
vfs_adddev(const char *devname, struct device *dev, int mountable)
{
	return vfs_doadd(devname, mountable, dev, NULL);
}

/*
 * Add a filesystem that does not have an underlying device.
 * This is used for emufs, but might also be used for network
 * filesystems and the like.
 */
int
vfs_addfs(const char *devname, struct fs *fs)
{
	return vfs_doadd(devname, 0, NULL, fs);
}

//////////////////////////////////////////////////

/*
 * Look for a mountable device named DEVNAME.
 * Should already hold knowndevs_lock.
 */
static
int
findmount(const char *devname, struct knowndev **result)
{
	struct knowndev *dev;
	unsigned i, num;
	bool found = false;

	KASSERT(vfs_biglock_do_i_hold());

	num = knowndevarray_num(knowndevs);
	for (i=0; !found && i<num; i++) {
		dev = knowndevarray_get(knowndevs, i);
		if (dev->kd_rawname==NULL) {
			/* not mountable/unmountable */
			continue;
		}

		if (!strcmp(devname, dev->kd_name)) {
			*result = dev;
			found = true;
		}
	}

	return found ? 0 : ENODEV;
}

/*
 * Mount a filesystem. Once we've found the device, call MOUNTFUNC to
 * set up the filesystem and hand back a struct fs.
 *
 * The DATA argument is passed through unchanged to MOUNTFUNC.
 */
int
vfs_mount(const char *devname, void *data,
	  int (*mountfunc)(void *data, struct device *, struct fs **ret))
{
	const char *volname;
	struct knowndev *kd;
	struct fs *fs;
	int result;

	vfs_biglock_acquire();

	result = findmount(devname, &kd);
	if (result) {
		vfs_biglock_release();
		return result;
	}

	if (kd->kd_fs != NULL) {
		vfs_biglock_release();
		return EBUSY;
	}
	KASSERT(kd->kd_rawname != NULL);
	KASSERT(kd->kd_device != NULL);

	result = mountfunc(data, kd->kd_device, &fs);
	if (result) {
		vfs_biglock_release();
		return result;
	}

	KASSERT(fs != NULL);
	KASSERT(fs != SWAP_FS); 

	kd->kd_fs = fs;

	volname = FSOP_GETVOLNAME(fs);
	kprintf("vfs: Mounted %s: on %s\n",
		volname ? volname : kd->kd_name, kd->kd_name);

	vfs_biglock_release();
	return 0;
}

/*
 * Like mount, but for attaching swap. Hands back the raw device
 * vnode. Unlike mount tolerates a trailing colon on the device name,
 * to avoid student-facing confusion.
 */
int
vfs_swapon(const char *devname, struct vnode **ret)
{
	char *myname = NULL;
	size_t len;
	struct knowndev *kd;
	int result;

	len = strlen(devname);
	if (len > 0 && devname[len - 1] == ':') {
		/* tolerate trailing :, e.g. lhd0: rather than lhd0 */
		myname = kstrdup(devname);
		if (myname == NULL) {
			return ENOMEM;
		}
		myname[len - 1] = 0;
		devname = myname;
	}

	vfs_biglock_acquire();

	result = findmount(devname, &kd);
	if (result) {
		goto out;
	}

	if (kd->kd_fs != NULL) {
		result = EBUSY;
		goto out;
	}
	KASSERT(kd->kd_rawname != NULL);
	KASSERT(kd->kd_device != NULL);

	kprintf("vfs: Swap attached to %s\n", kd->kd_name);

	kd->kd_fs = SWAP_FS;
	VOP_INCREF(kd->kd_vnode);
	*ret = kd->kd_vnode;

 out:
	vfs_biglock_release();
	if (myname != NULL) {
		kfree(myname);
	}

	return result;
}

/*
 * Unmount a filesystem/device by name.
 * First calls FSOP_SYNC on the filesystem; then calls FSOP_UNMOUNT.
 */
int
vfs_unmount(const char *devname)
{
	struct knowndev *kd;
	int result;

	vfs_biglock_acquire();

	result = findmount(devname, &kd);
	if (result) {
		goto fail;
	}

	if (kd->kd_fs == NULL || kd->kd_fs == SWAP_FS) {
		result = EINVAL;
		goto fail;
	}
	KASSERT(kd->kd_rawname != NULL);
	KASSERT(kd->kd_device != NULL);

	/* sync the fs */
	result = FSOP_SYNC(kd->kd_fs);
	if (result) {
		goto fail;
	}

	result = FSOP_UNMOUNT(kd->kd_fs);
	if (result) {
		goto fail;
	}

	kprintf("vfs: Unmounted %s:\n", kd->kd_name);

	/* now drop the filesystem */
	kd->kd_fs = NULL;

	KASSERT(result==0);

 fail:
	vfs_biglock_release();
	return result;
}

/*
 * Detach swap. Like unmount.
 *
 * (Provided for completeness; there is no real need to remove swap
 * explicitly prior to shutting down, except perhaps when swapping to
 * things that themselves want a clean shutdown, like RAIDs.)
 */
int
vfs_swapoff(const char *devname)
{
	struct knowndev *kd;
	int result;

	vfs_biglock_acquire();

	result = findmount(devname, &kd);
	if (result) {
		goto fail;
	}

	if (kd->kd_fs != SWAP_FS) {
		result = EINVAL;
		goto fail;
	}

	kprintf("vfs: Swap detached from %s:\n", kd->kd_name);

	/* drop it */
	kd->kd_fs = NULL;

	KASSERT(result==0);

 fail:
	vfs_biglock_release();
	return result;
}

/*
 * Global unmount function.
 */
int
vfs_unmountall(void)
{
	struct knowndev *dev;
	unsigned i, num;
	int result;

	vfs_biglock_acquire();

	num = knowndevarray_num(knowndevs);
	for (i=0; i<num; i++) {
		dev = knowndevarray_get(knowndevs, i);
		if (dev->kd_rawname == NULL) {
			/* not mountable/unmountable */
			continue;
		}
		if (dev->kd_fs == NULL) {
			/* not mounted */
			continue;
		}
		if (dev->kd_fs == SWAP_FS) {
			/* just drop it */
			dev->kd_fs = NULL;
			continue;
		}

		kprintf("vfs: Unmounting %s:\n", dev->kd_name);

		result = FSOP_SYNC(dev->kd_fs);
		if (result) {
			kprintf("vfs: Warning: sync failed for %s: %s, trying "
				"again\n", dev->kd_name, strerror(result));

			result = FSOP_SYNC(dev->kd_fs);
			if (result) {
				kprintf("vfs: Warning: sync failed second time"
					" for %s: %s, giving up...\n",
					dev->kd_name, strerror(result));
				/*
				 * Do not attempt to complete the
				 * unmount as it will likely explode.
				 */
				continue;
			}
		}

		result = FSOP_UNMOUNT(dev->kd_fs);
		if (result == EBUSY) {
			kprintf("vfs: Cannot unmount %s: (busy)\n",
				dev->kd_name);
			continue;
		}
		if (result) {
			kprintf("vfs: Warning: unmount failed for %s:"
				" %s, already synced, dropping...\n",
				dev->kd_name, strerror(result));
			continue;
		}

		/* now drop the filesystem */
		dev->kd_fs = NULL;
	}

	vfs_biglock_release();

	return 0;
}
