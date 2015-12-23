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
#include <vnode.h>

/*
 * Routines that fail.
 *
 * It is kind of silly to write these out each with their particular
 * arguments; however, portable C doesn't let you cast function
 * pointers with different argument signatures even if the arguments
 * are never used.
 *
 * The 4.4BSD approach (all vnode ops take a void pointer that's cast
 * to a op-specific args structure, so they're all the same type)
 * avoids this problem but is otherwise not very appealing.
 */

////////////////////////////////////////////////////////////
// uio ops (read, readlink, getdirentry, write, namefile)

int
vopfail_uio_notdir(struct vnode *vn, struct uio *uio)
{
	(void)vn;
	(void)uio;
	return ENOTDIR;
}

int
vopfail_uio_isdir(struct vnode *vn, struct uio *uio)
{
	(void)vn;
	(void)uio;
	return EISDIR;
}

int
vopfail_uio_inval(struct vnode *vn, struct uio *uio)
{
	(void)vn;
	(void)uio;
	return EINVAL;
}

int
vopfail_uio_nosys(struct vnode *vn, struct uio *uio)
{
	(void)vn;
	(void)uio;
	return ENOSYS;
}

////////////////////////////////////////////////////////////
// mmap

int
vopfail_mmap_isdir(struct vnode *vn /*add stuff */)
{
	(void)vn;
	return EISDIR;
}

int
vopfail_mmap_perm(struct vnode *vn /*add stuff */)
{
	(void)vn;
	return EPERM;
}

int
vopfail_mmap_nosys(struct vnode *vn /*add stuff */)
{
	(void)vn;
	return ENOSYS;
}

////////////////////////////////////////////////////////////
// truncate

int
vopfail_truncate_isdir(struct vnode *vn, off_t pos)
{
	(void)vn;
	(void)pos;
	return EISDIR;
}

////////////////////////////////////////////////////////////
// creat

int
vopfail_creat_notdir(struct vnode *vn, const char *name, bool excl,
		     mode_t mode, struct vnode **result)
{
	(void)vn;
	(void)name;
	(void)excl;
	(void)mode;
	(void)result;
	return ENOTDIR;
}

////////////////////////////////////////////////////////////
// symlink

int
vopfail_symlink_notdir(struct vnode *vn, const char *contents,
		       const char *name)
{
	(void)vn;
	(void)contents;
	(void)name;
	return ENOTDIR;
}

int
vopfail_symlink_nosys(struct vnode *vn, const char *contents,
		      const char *name)
{
	(void)vn;
	(void)contents;
	(void)name;
	return ENOSYS;
}

////////////////////////////////////////////////////////////
// mkdir

int
vopfail_mkdir_notdir(struct vnode *vn, const char *name, mode_t mode)
{
	(void)vn;
	(void)name;
	(void)mode;
	return ENOTDIR;
}

int
vopfail_mkdir_nosys(struct vnode *vn, const char *name, mode_t mode)
{
	(void)vn;
	(void)name;
	(void)mode;
	return ENOSYS;
}

////////////////////////////////////////////////////////////
// link

int
vopfail_link_notdir(struct vnode *dir, const char *name, struct vnode *file)
{
	(void)dir;
	(void)name;
	(void)file;
	return ENOTDIR;
}

int
vopfail_link_nosys(struct vnode *dir, const char *name, struct vnode *file)
{
	(void)dir;
	(void)name;
	(void)file;
	return ENOSYS;
}

////////////////////////////////////////////////////////////
// string ops (remove and rmdir)

int
vopfail_string_notdir(struct vnode *vn, const char *name)
{
	(void)vn;
	(void)name;
	return ENOTDIR;
}

int
vopfail_string_nosys(struct vnode *vn, const char *name)
{
	(void)vn;
	(void)name;
	return ENOSYS;
}

////////////////////////////////////////////////////////////
// rename

int
vopfail_rename_notdir(struct vnode *fromdir, const char *fromname,
		      struct vnode *todir, const char *toname)
{
	(void)fromdir;
	(void)fromname;
	(void)todir;
	(void)toname;
	return ENOTDIR;
}

int
vopfail_rename_nosys(struct vnode *fromdir, const char *fromname,
		     struct vnode *todir, const char *toname)
{
	(void)fromdir;
	(void)fromname;
	(void)todir;
	(void)toname;
	return ENOSYS;
}

////////////////////////////////////////////////////////////
// lookup

int
vopfail_lookup_notdir(struct vnode *vn, char *path, struct vnode **result)
{
	(void)vn;
	(void)path;
	(void)result;
	return ENOTDIR;
}

int
vopfail_lookparent_notdir(struct vnode *vn, char *path, struct vnode **result,
			  char *buf, size_t len)
{
	(void)vn;
	(void)path;
	(void)result;
	(void)buf;
	(void)len;
	return ENOTDIR;
}

