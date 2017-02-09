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
 * VFS operations relating to pathname translation
 */

#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <lib.h>
#include <synch.h>
#include <vfs.h>
#include <fs.h>
#include <vnode.h>

static struct vnode *bootfs_vnode = NULL;

/*
 * Helper function for actually changing bootfs_vnode.
 */
static
void
change_bootfs(struct vnode *newvn)
{
	struct vnode *oldvn;

	oldvn = bootfs_vnode;
	bootfs_vnode = newvn;

	if (oldvn != NULL) {
		VOP_DECREF(oldvn);
	}
}

/*
 * Set bootfs_vnode.
 *
 * Bootfs_vnode is the vnode used for beginning path translation of
 * pathnames starting with /.
 *
 * It is also incidentally the system's first current directory.
 */
int
vfs_setbootfs(const char *fsname)
{
	char tmp[NAME_MAX+1];
	char *s;
	int result;
	struct vnode *newguy;

	vfs_biglock_acquire();

	snprintf(tmp, sizeof(tmp)-1, "%s", fsname);
	s = strchr(tmp, ':');
	if (s) {
		/* If there's a colon, it must be at the end */
		if (strlen(s)>0) {
			vfs_biglock_release();
			return EINVAL;
		}
	}
	else {
		strcat(tmp, ":");
	}

	result = vfs_chdir(tmp);
	if (result) {
		vfs_biglock_release();
		return result;
	}

	result = vfs_getcurdir(&newguy);
	if (result) {
		vfs_biglock_release();
		return result;
	}

	change_bootfs(newguy);

	vfs_biglock_release();
	return 0;
}

/*
 * Clear the bootfs vnode (preparatory to system shutdown).
 */
void
vfs_clearbootfs(void)
{
	vfs_biglock_acquire();
	change_bootfs(NULL);
	vfs_biglock_release();
}


/*
 * Common code to pull the device name, if any, off the front of a
 * path and choose the vnode to begin the name lookup relative to.
 */

static
int
getdevice(char *path, char **subpath, struct vnode **startvn)
{
	int slash=-1, colon=-1, i;
	struct vnode *vn;
	int result;

	KASSERT(vfs_biglock_do_i_hold());

	/*
	 * Entirely empty filenames aren't legal.
	 */
	if (path[0] == 0) {
		return EINVAL;
	}

	/*
	 * Locate the first colon or slash.
	 */

	for (i=0; path[i]; i++) {
		if (path[i]==':') {
			colon = i;
			break;
		}
		if (path[i]=='/') {
			slash = i;
			break;
		}
	}

	if (colon < 0 && slash != 0) {
		/*
		 * No colon before a slash, so no device name
		 * specified, and the slash isn't leading or is also
		 * absent, so this is a relative path or just a bare
		 * filename. Start from the current directory, and
		 * use the whole thing as the subpath.
		 */
		*subpath = path;
		return vfs_getcurdir(startvn);
	}

	if (colon>0) {
		/* device:path - get root of device's filesystem */
		path[colon]=0;
		while (path[colon+1]=='/') {
			/* device:/path - skip slash, treat as device:path */
			colon++;
		}
		*subpath = &path[colon+1];

		result = vfs_getroot(path, startvn);
		if (result) {
			return result;
		}

		return 0;
	}

	/*
	 * We have either /path or :path.
	 *
	 * /path is a path relative to the root of the "boot filesystem".
	 * :path is a path relative to the root of the current filesystem.
	 */
	KASSERT(colon==0 || slash==0);

	if (path[0]=='/') {
		if (bootfs_vnode==NULL) {
			return ENOENT;
		}
		VOP_INCREF(bootfs_vnode);
		*startvn = bootfs_vnode;
	}
	else {
		KASSERT(path[0]==':');

		result = vfs_getcurdir(&vn);
		if (result) {
			return result;
		}

		/*
		 * The current directory may not be a device, so it
		 * must have a fs.
		 */
		KASSERT(vn->vn_fs!=NULL);

		result = FSOP_GETROOT(vn->vn_fs, startvn);

		VOP_DECREF(vn);

		if (result) {
			return result;
		}
	}

	while (path[1]=='/') {
		/* ///... or :/... */
		path++;
	}

	*subpath = path+1;

	return 0;
}

/*
 * Name-to-vnode translation.
 * (In BSD, both of these are subsumed by namei().)
 */

int
vfs_lookparent(char *path, struct vnode **retval,
	       char *buf, size_t buflen)
{
	struct vnode *startvn;
	int result;

	vfs_biglock_acquire();

	result = getdevice(path, &path, &startvn);
	if (result) {
		vfs_biglock_release();
		return result;
	}

	if (strlen(path)==0) {
		/*
		 * It does not make sense to use just a device name in
		 * a context where "lookparent" is the desired
		 * operation.
		 */
		result = EINVAL;
	}
	else {
		result = VOP_LOOKPARENT(startvn, path, retval, buf, buflen);
	}

	VOP_DECREF(startvn);

	vfs_biglock_release();
	return result;
}

int
vfs_lookup(char *path, struct vnode **retval)
{
	struct vnode *startvn;
	int result;

	vfs_biglock_acquire();

	result = getdevice(path, &path, &startvn);
	if (result) {
		vfs_biglock_release();
		return result;
	}

	if (strlen(path)==0) {
		*retval = startvn;
		vfs_biglock_release();
		return 0;
	}

	result = VOP_LOOKUP(startvn, path, retval);

	VOP_DECREF(startvn);
	vfs_biglock_release();
	return result;
}
