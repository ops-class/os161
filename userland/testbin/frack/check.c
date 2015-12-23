/*
 * Copyright (c) 2013, 2015
 *	The President and Fellows of Harvard College.
 *      Written by David A. Holland.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <err.h>

#include "name.h"
#include "data.h"
#include "pool.h"
#include "check.h"

/*
 * Check pass.
 *
 * We first replay the workload into a model that keeps track of what
 * should be on the volume at each step; then we inspect the real
 * volume and compare it to the model.
 */

////////////////////////////////////////////////////////////
// model representation

#define UNKNOWN_ID ((unsigned)-1)

/*
 * 1. log of changes to a filesystem
 * (this is not quite fully general but supports only things we do)
 */

enum fschanges {
	FC_NEWFS,		/* create the volume */
	FC_TRUNCATE,		/* truncate a file */
	FC_WRITE,		/* write to a file */
	FC_CREAT,		/* create a file */
	FC_MKDIR,		/* create a directory */
	FC_RMDIR,		/* remove a directory */
	FC_UNLINK,		/* remove a file */
	FC_LINK,		/* hardlink a file */
	FC_RENAMEFILE,		/* rename a file */
	FC_RENAMEDIR,		/* rename a directory */
};
struct fschange {
	/* all changes are kept in order on a doubly linked list */
	struct fschange *prev;
	struct fschange *next;

	/* version number for this change */
	unsigned version;

	/* whether this change reflects a partially committed operation */
	int partial;

	/* the change type, parameters, and metadata */
	enum fschanges type;
	union {
		struct {
			/* object id of the root directory */
			unsigned rootdirnum;
		} fc_newfs;
		struct {
			/* cached: previous change affecting this file */
			struct fschange *prev_thisfile;
			/* truncate params: file oid and new length */
			unsigned file;
			off_t len;
		} fc_truncate;
		struct {
			/* cached: previous change affecting this file */
			struct fschange *prev_thisfile;
			/* write params: file oid, position, length */
			unsigned file;
			off_t pos;
			off_t len;
			/* size of file before the write */
			off_t oldfilesize;
			/* key for generating the write contents */
			unsigned code;
			unsigned seq;
		} fc_write;
		struct {
			/* cached: previous change affecting containing dir */
			struct fschange *prev_thisdir;
			/* creat params: containing dir oid, name, new oid */
			unsigned dir;
			unsigned name;
			unsigned newfile;
		} fc_creat;
		struct {
			/* cached: previous change affecting containing dir */
			struct fschange *prev_thisdir;
			/* mkdir params: containing dir oid, name, new oid */
			unsigned dir;
			unsigned name;
			unsigned newdir;
		} fc_mkdir;
		struct {
			/* cached: previous change affecting either dir */
			struct fschange *prev_thisdir;
			struct fschange *prev_victimdir;
			/* rmdir params */
			unsigned dir;
			unsigned name;
			unsigned victimdir;
		} fc_rmdir;
		struct {
			/* cached: previous change affecting either object */
			struct fschange *prev_thisdir;
			struct fschange *prev_victimfile;
			/* unlink params */
			unsigned dir;
			unsigned name;
			unsigned victimfile;
		} fc_unlink;
		struct {
			/* cached: previous change affecting each object */
			struct fschange *prev_fromdir;
			struct fschange *prev_todir;
			struct fschange *prev_thisfile;
			/* link params */
			unsigned fromdir;
			unsigned fromname;
			unsigned todir;
			unsigned toname;
			unsigned file;
		} fc_link;
		struct {
			/* cached: previous change affecting each object */
			struct fschange *prev_fromdir;
			struct fschange *prev_todir;
			struct fschange *prev_movedfile;
			/* rename params */
			unsigned fromdir;
			unsigned fromname;
			unsigned todir;
			unsigned toname;
			unsigned movedfile;
		} fc_renamefile;
		struct {
			/* cached: previous change affecting each object */
			struct fschange *prev_fromdir;
			struct fschange *prev_todir;
			struct fschange *prev_moveddir;
			/* rename params */
			unsigned fromdir;
			unsigned fromname;
			unsigned todir;
			unsigned toname;
			unsigned moveddir;
		} fc_renamedir;
	};
};

/*
 * 2. representation of a current (visible) filesystem state
 *
 * The state starts at the root directory; each directory is a singly
 * linked list of directory entries, each of which holds another
 * directory or a file. We don't keep track of file contents, only the
 * length.
 *
 * The identity field is within the union structure because file and
 * directory object ids are different namespaces.
 */

struct fsdirent {
	unsigned name;
	struct fsobject *obj;
	struct fsdirent *next;
};

struct fsobject {
	unsigned isdir;
	unsigned refcount;
	union {
		struct {
			unsigned identity;
			off_t len;
		} obj_file;
		struct {
			unsigned identity;
			struct fsdirent *entries;
			struct fsobject *parent;
		} obj_dir;
	};
};

////////////////////////////////////////////////////////////
// allocator pools

/*
 * Allocate from static space instead of using malloc, to help avoid
 * unnecessary lossage on kernels where malloc doesn't work.
 */

#define MAXCHANGES	16384
#define MAXOBJECTS	16384
#define MAXDIRENTS	16384

DECLPOOL(fschange, MAXCHANGES);
DECLPOOL(fsobject, MAXOBJECTS);
DECLPOOL(fsdirent, MAXDIRENTS);

static
struct fschange *
getchange(void)
{
	return POOLALLOC(fschange);
}

static
struct fsobject *
getobject(void)
{
	return POOLALLOC(fsobject);
}

static
struct fsdirent *
getdirent(void)
{
	return POOLALLOC(fsdirent);
}

#if 0
static
void
putchange(struct fschange *fc)
{
	POOLFREE(fschange, fc);
}
#endif

static
void
putobject(struct fsobject *obj)
{
	POOLFREE(fsobject, obj);
}

static
void
putdirent(struct fsdirent *dirent)
{
	POOLFREE(fsdirent, dirent);
}

////////////////////////////////////////////////////////////
// record constructors for change records

static
struct fschange *
fc_create(enum fschanges type)
{
	struct fschange *fc;

	fc = getchange();
	fc->prev = fc->next = NULL;
	fc->version = 0;
	fc->partial = 0;
	fc->type = type;
	return fc;
}

static
struct fschange *
fc_create_newfs(unsigned rootdirnum)
{
	struct fschange *fc;

	fc = fc_create(FC_NEWFS);
	fc->fc_newfs.rootdirnum = rootdirnum;
	return fc;
}

static
struct fschange *
fc_create_truncate(struct fschange *prev_thisfile, unsigned file, off_t len)
{
	struct fschange *fc;

	fc = fc_create(FC_TRUNCATE);
	fc->fc_truncate.prev_thisfile = prev_thisfile;
	fc->fc_truncate.file = file;
	fc->fc_truncate.len = len;
	return fc;
}

static
struct fschange *
fc_create_write(struct fschange *prev_thisfile, unsigned file,
		off_t pos, off_t len, off_t oldfilesize,
		unsigned code, unsigned seq)
{
	struct fschange *fc;

	fc = fc_create(FC_WRITE);
	fc->fc_write.prev_thisfile = prev_thisfile;
	fc->fc_write.file = file;
	fc->fc_write.pos = pos;
	fc->fc_write.len = len;
	fc->fc_write.oldfilesize = oldfilesize;
	fc->fc_write.code = code;
	fc->fc_write.seq = seq;
	return fc;
}

static
struct fschange *
fc_create_creat(struct fschange *prev_thisdir,
		unsigned dir, unsigned name, unsigned newfile)
{
	struct fschange *mdc;

	mdc = fc_create(FC_CREAT);
	mdc->fc_creat.prev_thisdir = prev_thisdir;
	mdc->fc_creat.dir = dir;
	mdc->fc_creat.name = name;
	mdc->fc_creat.newfile = newfile;
	return mdc;
}

static
struct fschange *
fc_create_mkdir(struct fschange *prev_thisdir,
		unsigned dir, unsigned name, unsigned newdir)
{
	struct fschange *mdc;

	mdc = fc_create(FC_MKDIR);
	mdc->fc_mkdir.prev_thisdir = prev_thisdir;
	mdc->fc_mkdir.dir = dir;
	mdc->fc_mkdir.name = name;
	mdc->fc_mkdir.newdir = newdir;
	return mdc;
}

static
struct fschange *
fc_create_rmdir(struct fschange *prev_thisdir, struct fschange *prev_victimdir,
		unsigned dir, unsigned name, unsigned victimdir)
{
	struct fschange *mdc;

	mdc = fc_create(FC_RMDIR);
	mdc->fc_rmdir.prev_thisdir = prev_thisdir;
	mdc->fc_rmdir.prev_victimdir = prev_victimdir;
	mdc->fc_rmdir.dir = dir;
	mdc->fc_rmdir.name = name;
	mdc->fc_rmdir.victimdir = victimdir;
	return mdc;
}

static
struct fschange *
fc_create_unlink(struct fschange *prev_thisdir,
		 struct fschange *prev_victimfile,
		 unsigned dir, unsigned name, unsigned victimfile)
{
	struct fschange *mdc;

	mdc = fc_create(FC_UNLINK);
	mdc->fc_unlink.prev_thisdir = prev_thisdir;
	mdc->fc_unlink.prev_victimfile = prev_victimfile;
	mdc->fc_unlink.dir = dir;
	mdc->fc_unlink.name = name;
	mdc->fc_unlink.victimfile = victimfile;
	return mdc;
}

static
struct fschange *
fc_create_link(struct fschange *prev_fromdir, struct fschange *prev_todir,
	       struct fschange *prev_thisfile,
	       unsigned fromdir, unsigned fromname,
	       unsigned todir, unsigned toname,
	       unsigned file)
{
	struct fschange *mdc;

	mdc = fc_create(FC_LINK);
	mdc->fc_link.prev_fromdir = prev_fromdir;
	mdc->fc_link.prev_todir = prev_todir;
	mdc->fc_link.prev_thisfile = prev_thisfile;
	mdc->fc_link.fromdir = fromdir;
	mdc->fc_link.fromname = fromname;
	mdc->fc_link.todir = todir;
	mdc->fc_link.toname = toname;
	mdc->fc_link.file = file;
	return mdc;
}

static
struct fschange *
fc_create_renamefile(struct fschange *prev_fromdir,
		     struct fschange *prev_todir,
		     struct fschange *prev_movedfile,
		     unsigned fromdir, unsigned fromname,
		     unsigned todir, unsigned toname,
		     unsigned movedfile)
{
	struct fschange *mdc;

	mdc = fc_create(FC_RENAMEFILE);
	mdc->fc_renamefile.prev_fromdir = prev_fromdir;
	mdc->fc_renamefile.prev_todir = prev_todir;
	mdc->fc_renamefile.prev_movedfile = prev_movedfile;
	mdc->fc_renamefile.fromdir = fromdir;
	mdc->fc_renamefile.fromname = fromname;
	mdc->fc_renamefile.todir = todir;
	mdc->fc_renamefile.toname = toname;
	mdc->fc_renamefile.movedfile = movedfile;
	return mdc;
}

static
struct fschange *
fc_create_renamedir(struct fschange *prev_fromdir,
		    struct fschange *prev_todir,
		    struct fschange *prev_moveddir,
		    unsigned fromdir, unsigned fromname,
		    unsigned todir, unsigned toname,
		    unsigned moveddir)
{
	struct fschange *fc;

	fc = fc_create(FC_RENAMEDIR);
	fc->fc_renamedir.prev_fromdir = prev_fromdir;
	fc->fc_renamedir.prev_todir = prev_todir;
	fc->fc_renamedir.prev_moveddir = prev_moveddir;
	fc->fc_renamedir.fromdir = fromdir;
	fc->fc_renamedir.fromname = fromname;
	fc->fc_renamedir.todir = todir;
	fc->fc_renamedir.toname = toname;
	fc->fc_renamedir.moveddir = moveddir;
	return fc;
}

////////////////////////////////////////////////////////////
// constructors/destructors for current state objects

static
struct fsdirent *
fsdirent_create(unsigned name, struct fsobject *obj)
{
	struct fsdirent *ret;

	ret = getdirent();
	ret->name = name;
	ret->obj = obj;
	ret->next = NULL;
	return ret;
}

static
void
fsdirent_destroy(struct fsdirent *de)
{
	assert(de->obj == NULL);
	assert(de->next == NULL);
	putdirent(de);
}

static
struct fsobject *
fsobject_create_file(unsigned id)
{
	struct fsobject *ret;

	ret = getobject();
	ret->isdir = 0;
	ret->refcount = 1;
	ret->obj_file.identity = id;
	ret->obj_file.len = 0;
	return ret;
}

static
struct fsobject *
fsobject_create_dir(unsigned id, struct fsobject *parent)
{
	struct fsobject *ret;

	ret = getobject();
	ret->isdir = 1;
	ret->refcount = 1;
	ret->obj_dir.identity = id;
	ret->obj_dir.entries = NULL;
	ret->obj_dir.parent = parent;
	return ret;
}

static
void
fsobject_incref(struct fsobject *obj)
{
	assert(obj->refcount > 0);
	obj->refcount++;
	assert(obj->refcount > 0);
}

static
void
fsobject_decref(struct fsobject *obj)
{
	assert(obj->refcount > 0);
	obj->refcount--;
	if (obj->refcount > 0) {
		return;
	}

	if (obj->isdir) {
		assert(obj->obj_dir.entries == NULL);
	}
	putobject(obj);
}

static
void
fsobject_destroytree(struct fsobject *obj)
{
	struct fsdirent *de;

	if (obj->isdir) {
		while (obj->obj_dir.entries != NULL) {
			de = obj->obj_dir.entries;
			obj->obj_dir.entries = de->next;
			de->next = NULL;
			fsobject_destroytree(de->obj);
			de->obj = NULL;
			fsdirent_destroy(de);
		}
	}
	fsobject_decref(obj);
}

////////////////////////////////////////////////////////////
// operations on current state objects

/*
 * Bail out if something's broken.
 */
static
void
die(const char *fmt, ...)
{
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	errx(1, "Inconsistency: %s", buf);
}

#if 0 /* not used */
/*
 * Count a list of directory entries.
 */
static
unsigned
fsdirent_count(struct fsdirent *de)
{
	if (de == NULL) {
		return 0;
	}
	return 1 + fsdirent_count(de->next);
}
#endif

/*
 * Add an entry to a directory.
 */
static
void
fsdir_add_entry(struct fsobject *dir, struct fsdirent *nde)
{
	struct fsdirent *ode;

	assert(dir->isdir);
	for (ode = dir->obj_dir.entries; ode != NULL; ode = ode->next) {
		if (ode->name == nde->name) {
			die("In directory %u, %s already existed",
			    dir->obj_dir.identity, name_get(nde->name));
		}
	}
	nde->next = dir->obj_dir.entries;
	dir->obj_dir.entries = nde;
}

/*
 * Find an entry in a directory by name. If CROAK is set, bail out if
 * it's not there.
 */
static
struct fsdirent *
fsdir_find_entry(struct fsobject *dir, unsigned name, int croak)
{
	struct fsdirent *de;

	assert(dir->isdir);
	for (de = dir->obj_dir.entries; de != NULL; de = de->next) {
		if (de->name == name) {
			return de;
		}
	}
	if (croak) {
		die("In directory %u, did not find %s",
		    dir->obj_dir.identity, name_get(name));
	}
	return NULL;
}

/*
 * Remove an entry from a directory.
 */
static
struct fsdirent *
fsdir_remove_entry(struct fsobject *dir, unsigned name)
{
	struct fsdirent *de, **prevptr;

	assert(dir->isdir);
	prevptr = &dir->obj_dir.entries;
	for (de = *prevptr; de != NULL; prevptr = &de->next, de = *prevptr) {
		if (de->name == name) {
			*prevptr = de->next;
			de->next = NULL;
			return de;
		}
	}
	die("In directory %u, did not find %s",
	    dir->obj_dir.identity, name_get(name));
	return NULL;
}

////////////////////////////////////////////////////////////
// apply a change record to a current state

static
struct fsobject *
findsub(struct fsobject *obj, unsigned isdir, unsigned id)
{
	struct fsobject *ret;
	struct fsdirent *de;
	unsigned objid;

	/* Are we on the object we're looking for? */
	objid = obj->isdir ? obj->obj_dir.identity : obj->obj_file.identity;
	if (obj->isdir == isdir && objid == id) {
		return obj;
	}

	/* If the object we're on isn't a dir, can't search any further */
	if (!obj->isdir) {
		return NULL;
	}

	for (de = obj->obj_dir.entries; de != NULL; de = de->next) {
		ret = findsub(de->obj, isdir, id);
		if (ret != NULL) {
			return ret;
		}
	}
	return NULL;
}

#if 0
static
struct fsobject *
findfile(struct fsobject *rootdir, unsigned id)
{
	struct fsobject *ret;

	ret = findsub(rootdir, 0/*!isdir*/, id);
	if (ret == NULL) {
		die("File %u not found in current state", id);
	}
	return ret;
}
#endif

static
struct fsobject *
findfile_maybe(struct fsobject *rootdir, unsigned id)
{
	return findsub(rootdir, 0/*!isdir*/, id);
}

static
struct fsobject *
finddir(struct fsobject *rootdir, unsigned id)
{
	struct fsobject *ret;

	ret = findsub(rootdir, 1/*isdir*/, id);
	if (ret == NULL) {
		die("Directory %u not found in current state", id);
	}
	return ret;
}

/*
 * Apply change CHANGE to the volume state encoded in/under ROOTDIR.
 */
static
void
apply_change(struct fsobject *rootdir, struct fschange *change)
{
	struct fsobject *obj1, *obj2;
	struct fsdirent *de;
	off_t endpos;

	assert(rootdir->isdir);

	switch (change->type) {
	    case FC_NEWFS:
		/*
		 * Creating the volume; crosscheck the root directory.
		 */
		assert(rootdir->isdir);
		assert(rootdir->refcount == 1);
		assert(rootdir->obj_dir.identity ==
		       change->fc_newfs.rootdirnum);
		assert(rootdir->obj_dir.entries == NULL);
		assert(rootdir->obj_dir.parent == rootdir);
		break;
	    case FC_TRUNCATE:
		/*
		 * Truncate a file. (Only files, not directories.)
		 *
		 * Because truncates can and do get posted after a
		 * file is unlinked, we have to tolerate not finding
		 * the file. We don't model unlinked files, because
		 * they aren't visible and our goal is to check the
		 * visible/observable state.
		 */
		obj1 = findfile_maybe(rootdir, change->fc_truncate.file);
		if (obj1 != NULL) {
			obj1->obj_file.len = change->fc_truncate.len;
		}
		break;
	    case FC_WRITE:
		/*
		 * Write to a file. (Only files, not directories.)
		 * All this actually does is update the file size if
		 * needed.
		 *
		 * We also have to tolerate writes to unlinked files.
		 */
		obj1 = findfile_maybe(rootdir, change->fc_write.file);
		if (obj1 != NULL) {
			endpos = change->fc_write.pos + change->fc_write.len;
			if (obj1->obj_file.len < endpos) {
				obj1->obj_file.len = endpos;
			}
		}
		break;
	    case FC_CREAT:
		/*
		 * Create a file. This creates a new object and a
		 * directory entry to hold it, then inserts the
		 * directory entry in the containing directory.
		 */
		obj1 = finddir(rootdir, change->fc_creat.dir);
		obj2 = fsobject_create_file(change->fc_creat.newfile);
		de = fsdirent_create(change->fc_creat.name, obj2);
		fsdir_add_entry(obj1, de);
		break;
	    case FC_MKDIR:
		/*
		 * Create a directory. The same as creating a file,
		 * except the new object is a directory.
		 */
		obj1 = finddir(rootdir, change->fc_mkdir.dir);
		obj2 = fsobject_create_dir(change->fc_mkdir.newdir, obj1);
		de = fsdirent_create(change->fc_mkdir.name, obj2);
		fsdir_add_entry(obj1, de);
		break;
	    case FC_RMDIR:
		/*
		 * Remove a directory. First take out the directory
		 * entry (by name); then examine the object found;
		 * then delete everything.
		 *
		 * XXX: why do these checks use assert vs. testing and
		 * calling die()?
		 */
		obj1 = finddir(rootdir, change->fc_rmdir.dir);
		de = fsdir_remove_entry(obj1, change->fc_rmdir.name);
		obj2 = de->obj;
		de->obj = NULL;
		assert(obj2->isdir);
		assert(obj2->obj_dir.entries == NULL);
		assert(obj2->obj_dir.identity == change->fc_rmdir.victimdir);
		assert(obj2->obj_dir.parent == obj1);
		fsdirent_destroy(de);
		fsobject_decref(obj2);
		break;
	    case FC_UNLINK:
		/*
		 * Remove a file. Much the same as removing a directory.
		 */
		obj1 = finddir(rootdir, change->fc_unlink.dir);
		de = fsdir_remove_entry(obj1, change->fc_unlink.name);
		obj2 = de->obj;
		de->obj = NULL;
		assert(!obj2->isdir);
		assert(obj2->obj_file.identity ==
		       change->fc_unlink.victimfile);
		fsdirent_destroy(de);
		fsobject_decref(obj2);
		break;
	    case FC_LINK:
		/*
		 * Hard-link a file.
		 */
		obj1 = finddir(rootdir, change->fc_link.fromdir);
		de = fsdir_find_entry(obj1, change->fc_link.fromname, 1);
		obj2 = de->obj;
		assert(!obj2->isdir);
		assert(obj2->obj_file.identity == change->fc_link.file);
		obj1 = finddir(rootdir, change->fc_link.todir);
		fsobject_incref(obj2);
		de = fsdirent_create(change->fc_link.toname, obj2);
		fsdir_add_entry(obj1, de);
		break;
	    case FC_RENAMEFILE:
		/*
		 * Rename a file. XXX: this appears to do the wrong
		 * thing if you rename one file over another.
		 */
		obj1 = finddir(rootdir, change->fc_renamefile.fromdir);
		de = fsdir_remove_entry(obj1, change->fc_renamefile.fromname);
		obj2 = de->obj;
		assert(!obj2->isdir);
		assert(obj2->obj_file.identity ==
		       change->fc_renamefile.movedfile);
		obj1 = finddir(rootdir, change->fc_renamefile.todir);
		de->name = change->fc_renamefile.toname;
		fsdir_add_entry(obj1, de);
		break;
	    case FC_RENAMEDIR:
		/*
		 * Rename a directory. Same as renaming a file, except
		 * that we update the parent of the directory being
		 * moved. XXX: also seems to do the wrong thing if you
		 * rename one directory over another. And, XXX: should
		 * this be updating the refcount as the parent
		 * changes? Shouldn't the refcount be the same as the
		 * on-disk linkcount, and shouldn't the on-disk
		 * linkcount be part of the state we examine and
		 * verify?
		 */
		obj1 = finddir(rootdir, change->fc_renamedir.fromdir);
		de = fsdir_remove_entry(obj1, change->fc_renamedir.fromname);
		obj2 = de->obj;
		assert(obj2->isdir);
		assert(obj2->obj_dir.identity ==
		       change->fc_renamedir.moveddir);
		assert(obj2->obj_dir.parent == obj1);
		obj1 = finddir(rootdir, change->fc_renamedir.todir);
		de->name = change->fc_renamedir.toname;
		obj2->obj_dir.parent = obj1;
		fsdir_add_entry(obj1, de);
		break;
	}
}

////////////////////////////////////////////////////////////
// global fs state

/*
 * The change records. FIRSTCHANGE is the first change record; CHANGES
 * is the list root, which given the current linked list implementation
 * is the *last* (most recent) change record.
 */
static struct fschange *firstchange;
static struct fschange *changes;

/*
 * The current volume state that we're working with. As we add change
 * records while replaying the workload, we roll this forward so we
 * can inspect it; for some things this is necessary to be able to
 * replay the workload fully.
 */
static struct fsobject *state;

/*
 * The next oids of each type to assign. File and directory ids are
 * counted independently, which might or might not have been a good
 * idea.
 */
static unsigned nextfilenum, nextdirnum;

/*
 * Add a new change record. This allocates the version number;
 * however, the change record should otherwise be fully filled in
 * already.
 *
 * usage: fc_attach(newrecord);
 */
static
void
fc_attach(struct fschange *new)
{
	struct fschange *prev;

	prev = changes;

	assert(prev->next == NULL);
	assert(new->prev == NULL);
	assert(new->next == NULL);
	prev->next = new;
	new->prev = prev;
	new->version = prev->version + 1;

	changes = new;
	apply_change(state, new);
}

/*
 * Rewind the volume state kept in the global STATE to the beginning.
 */
static
void
rewindstate(void)
{
	/* If we already have a state, throw it away. */
	if (state != NULL) {
		fsobject_destroytree(state);
	}

	assert(firstchange->type == FC_NEWFS);

	state = fsobject_create_dir(firstchange->fc_newfs.rootdirnum, NULL);
	/* root directory's parent is itself */
	state->obj_dir.parent = state;
}

/*
 * Roll the volume state kept in STATE forward to a specific change
 * entry.
 */
static
void
advancestateto(struct fschange *targetchange)
{
	struct fschange *change;

	for (change = firstchange; change != NULL; change = change->next) {
		apply_change(state, change);
		if (change == targetchange) {
			return;
		}
	}
	assert(0);
}

////////////////////////////////////////////////////////////
// lookup in the fs state
// (used during model construction)

/*
 * Find the most recent previous record that mentions a particular file.
 *
 * If there isn't one, something's wrong; because we start with a
 * fresh volume every file that ever exists must have been created at
 * some point.
 *
 * This is used while replaying the workload and accumulating change
 * records, so it can always just start from the most recent end
 * instead of from a specific place.
 */
static
struct fschange *
changes_findprevfile(unsigned filenum)
{
	struct fschange *here;

	for (here = changes; here != NULL; here = here->prev) {
		switch (here->type) {
		    case FC_NEWFS:
			break;
		    case FC_TRUNCATE:
			if (here->fc_truncate.file == filenum) {
				return here;
			}
			break;
		    case FC_WRITE:
			if (here->fc_write.file == filenum) {
				return here;
			}
			break;
		    case FC_CREAT:
			if (here->fc_creat.newfile == filenum) {
				return here;
			}
			break;
		    case FC_MKDIR:
		    case FC_RMDIR:
			break;
		    case FC_UNLINK:
			if (here->fc_unlink.victimfile == filenum) {
				return here;
			}
			break;
		    case FC_LINK:
			if (here->fc_link.file == filenum) {
				return here;
			}
			break;
		    case FC_RENAMEFILE:
			if (here->fc_renamefile.movedfile == filenum) {
				return here;
			}
			break;
		    case FC_RENAMEDIR:
			break;
		}
	}
	die("No previous record for file %u", filenum);
	return NULL;
}

/*
 * Find the most recent previous record that mentions a particular dir.
 *
 * Likewise, if there isn't one, something's wrong.
 */
static
struct fschange *
changes_findprevdir(unsigned dirnum)
{
	struct fschange *here;

	for (here = changes; here != NULL; here = here->prev) {
		switch (here->type) {
		    case FC_NEWFS:
			if (here->fc_newfs.rootdirnum == dirnum) {
				return here;
			}
			break;
		    case FC_TRUNCATE:
		    case FC_WRITE:
			break;
		    case FC_CREAT:
			if (here->fc_creat.dir == dirnum) {
				return here;
			}
			break;
		    case FC_MKDIR:
			if (here->fc_mkdir.dir == dirnum ||
			    here->fc_mkdir.newdir == dirnum) {
				return here;
			}
			break;
		    case FC_RMDIR:
			if (here->fc_rmdir.dir == dirnum ||
			    here->fc_rmdir.victimdir == dirnum) {
				return here;
			}
			break;
		    case FC_UNLINK:
			if (here->fc_unlink.dir == dirnum) {
				return here;
			}
			break;
		    case FC_LINK:
			if (here->fc_link.fromdir == dirnum ||
			    here->fc_link.todir == dirnum) {
				return here;
			}
			break;
		    case FC_RENAMEFILE:
			if (here->fc_renamefile.fromdir == dirnum ||
			    here->fc_renamefile.todir == dirnum) {
				return here;
			}
			break;
		    case FC_RENAMEDIR:
			if (here->fc_renamedir.fromdir == dirnum ||
			    here->fc_renamedir.todir == dirnum ||
			    here->fc_renamedir.moveddir == dirnum) {
				return here;
			}
			break;
		}
	}
	die("No previous record for directory %u", dirnum);
	return NULL;
}

#if 0
/*
 * Back up to the previous record mentioning the same file.
 *
 * XXX: No longer used and should be removed.
 */
static
struct fschange *
fs_prevforfile(struct fschange *fc, unsigned filenum)
{
	switch (fc->type) {
	    case FC_TRUNCATE:
		if (fc->fc_truncate.file == filenum) {
			return fc->fc_truncate.prev_thisfile;
		}
		break;
	    case FC_WRITE:
		if (fc->fc_write.file == filenum) {
			return fc->fc_write.prev_thisfile;
		}
		break;
	    case FC_UNLINK:
		if (fc->fc_unlink.victimfile == filenum) {
			return fc->fc_unlink.prev_victimfile;
		}
		break;
	    case FC_LINK:
		if (fc->fc_link.file == filenum) {
			return fc->fc_link.prev_thisfile;
		}
		break;
	    case FC_RENAMEFILE:
		if (fc->fc_renamefile.movedfile == filenum) {
			return fc->fc_renamefile.prev_movedfile;
		}
		break;
	    default:
		break;
	}
	return fc->prev;
}
#endif

#if 0
/*
 * Back up to the previous record mentioning the same directory.
 *
 * XXX: No longer used and should be removed.
 */
static
struct fschange *
fs_prevfordir(struct fschange *fc, unsigned dirnum)
{
	switch (fc->type) {
	    case FC_CREAT:
		if (fc->fc_creat.dir == dirnum) {
			return fc->fc_creat.prev_thisdir;
		}
		break;
	    case FC_MKDIR:
		if (fc->fc_mkdir.dir == dirnum) {
			return fc->fc_mkdir.prev_thisdir;
		}
		break;
	    case FC_RMDIR:
		if (fc->fc_rmdir.dir == dirnum) {
			return fc->fc_rmdir.prev_thisdir;
		}
		if (fc->fc_rmdir.victimdir == dirnum) {
			return fc->fc_rmdir.prev_victimdir;
		}
		break;
	    case FC_UNLINK:
		if (fc->fc_unlink.dir == dirnum) {
			return fc->fc_unlink.prev_thisdir;
		}
		break;
	    case FC_LINK:
		if (fc->fc_link.fromdir == dirnum) {
			return fc->fc_link.prev_fromdir;
		}
		if (fc->fc_link.todir == dirnum) {
			return fc->fc_link.prev_todir;
		}
		break;
	    case FC_RENAMEFILE:
		if (fc->fc_renamefile.fromdir == dirnum) {
			return fc->fc_renamefile.prev_fromdir;
		}
		if (fc->fc_renamefile.todir == dirnum) {
			return fc->fc_renamefile.prev_todir;
		}
		break;
	    case FC_RENAMEDIR:
		if (fc->fc_renamedir.fromdir == dirnum) {
			return fc->fc_renamedir.prev_fromdir;
		}
		if (fc->fc_renamedir.todir == dirnum) {
			return fc->fc_renamedir.prev_todir;
		}
		if (fc->fc_renamedir.moveddir == dirnum) {
			return fc->fc_renamedir.prev_moveddir;
		}
		break;
	    default:
		break;
	}
	return fc->prev;
}

/*
 * Find an object by name and containing directory by searching the
 * change history.
 *
 * No longer used (instead we maintain the current state and search
 * that, which is a lot simpler) and should probably be removed.
 */
struct findresult {
	int isfile;
	unsigned objnum;
};

static
int
fs_find(unsigned dirnum, unsigned name, struct findresult *res)
{
	struct fschange *here;

	for (here = fs; here != NULL; here = fs_prevfordir(here, dirnum)) {
		switch (here->type) {
		    case FC_NEWFS:
		    case FC_TRUNCATE:
		    case FC_WRITE:
			break;
		    case FC_CREAT:
			if (here->fc_creat.dir == dirnum &&
			    here->fc_creat.name == name) {
				res->isfile = 1;
				res->objnum = here->fc_creat.newfile;
				return 0;
			}
			break;
		    case FC_MKDIR:
			if (here->fc_mkdir.dir == dirnum &&
			    here->fc_mkdir.name == name) {
				res->isfile = 0;
				res->objnum = here->fc_mkdir.newdir;
				return 0;
			}
			if (here->fc_mkdir.newdir == dirnum) {
				return -1;
			}
			break;
		    case FC_RMDIR:
			if (here->fc_rmdir.dir == dirnum &&
			    here->fc_rmdir.name == name) {
				return -1;
			}
			if (here->fc_rmdir.victimdir == dirnum) {
				return -1;
			}
			break;
		    case FC_UNLINK:
			if (here->fc_unlink.dir == dirnum &&
			    here->fc_unlink.name == name) {
				return -1;
			}
			break;
		    case FC_LINK:
			if ((here->fc_link.todir == dirnum &&
			    here->fc_link.toname == name) ||
			    (here->fc_link.fromdir == dirnum &&
			     here->fc_link.fromname == name)) {
				res->isfile = 1;
				res->objnum = here->fc_link.file;
				return 0;
			}
			break;
		    case FC_RENAMEFILE:
			if (here->fc_renamefile.fromdir == dirnum &&
			    here->fc_renamefile.fromname == name) {
				return -1;
			}
			if (here->fc_renamefile.todir == dirnum &&
			    here->fc_renamefile.toname == name) {
				res->isfile = 1;
				res->objnum = here->fc_renamefile.movedfile;
				return 0;
			}
			break;
		    case FC_RENAMEDIR:
			if (here->fc_renamedir.fromdir == dirnum &&
			    here->fc_renamedir.fromname == name) {
				return -1;
			}
			if (here->fc_renamedir.todir == dirnum &&
			    here->fc_renamedir.toname == name) {
				res->isfile = 0;
				res->objnum = here->fc_renamedir.moveddir;
				return 0;
			}
			break;
		}
	}
	return -1;
}

/*
 * Find a file by name and containing directory, using fs_find to
 * search the history.
 *
 * No longer used; should be removed.
 */
static
unsigned
fs_findfile(unsigned dirnum, unsigned name)
{
	struct findresult res;

	if (fs_find(dirnum, name, &res) < 0) {
		die("In directory %u, did not find %s",
		    dirnum, name_get(name));
	}
	if (res.isfile == 0) {
		die("In directory %u, %s was a directory",
		    dirnum, name_get(name));
	}
	return res.objnum;
}

/*
 * Find a directory by name and containing directory, using fs_find to
 * search the history.
 *
 * No longer used; should be removed.
 */
static
unsigned
fs_finddir(unsigned dirnum, unsigned name)
{
	struct findresult res;

	if (fs_find(dirnum, name, &res) < 0) {
		die("In directory %u, did not find %s",
		    dirnum, name_get(name));
	}
	if (res.isfile == 1) {
		die("In directory %u, %s was not a directory",
		    dirnum, name_get(name));
	}
	return res.objnum;
}

/*
 * Check if a name (in a specific containing directory) is a file,
 * using fs_find.
 *
 * No longer used; should be removed.
 */
static
int
fs_isfile(unsigned dirnum, unsigned name)
{
	struct findresult res;

	if (fs_find(dirnum, name, &res) < 0) {
		return 0;
	}
	return res.isfile;
}

/*
 * Check if a name (in a specific containing directory) is a
 * directory, using fs_find.
 *
 * No longer used; should be removed.
 */
static
int
fs_isdir(unsigned dirnum, unsigned name)
{
	struct findresult res;

	if (fs_find(dirnum, name, &res) < 0) {
		return 0;
	}
	return !res.isfile;
}

/*
 * Find the parent of a directory by its object id, by searching the
 * history.
 *
 * No longer used; should be removed.
 */
static
unsigned
fs_findparent(unsigned dirnum)
{
	struct fschange *here;

	for (here = fs; here != NULL; here = fs_prevfordir(here, dirnum)) {
		switch (here->type) {
		    case FC_NEWFS:
			if (here->fc_newfs.rootdirnum == dirnum) {
				return dirnum;
			}
			break;
		    case FC_TRUNCATE:
		    case FC_WRITE:
		    case FC_CREAT:
			break;
		    case FC_MKDIR:
			if (here->fc_mkdir.newdir == dirnum) {
				return here->fc_mkdir.dir;
			}
			break;
		    case FC_RMDIR:
			if (here->fc_rmdir.victimdir == dirnum) {
				die("Directory %u was removed", dirnum);
			}
			break;
		    case FC_UNLINK:
		    case FC_LINK:
		    case FC_RENAMEFILE:
			break;
		    case FC_RENAMEDIR:
			if (here->fc_renamedir.moveddir == dirnum) {
				return here->fc_renamedir.todir;
			}
			break;
		}
	}
	die("Directory %u not found", dirnum);
	return 0;
}
#endif /* 0 */

/*
 * Find a file by name and containing directory, by searching the
 * volume state.
 */
static
unsigned
model_findfile(unsigned dirnum, unsigned name)
{
	struct fsobject *obj;
	struct fsdirent *de;

	obj = finddir(state, dirnum);
	de = fsdir_find_entry(obj, name, 1);
	if (de->obj->isdir) {
		die("In directory %u, %s was a directory",
		    dirnum, name_get(name));
	}
	return de->obj->obj_file.identity;
}

/*
 * Find a directory by name and containing directory, by searching the
 * volume state.
 */
static
unsigned
model_finddir(unsigned dirnum, unsigned name)
{
	struct fsobject *obj;
	struct fsdirent *de;

	obj = finddir(state, dirnum);
	de = fsdir_find_entry(obj, name, 1);
	if (!de->obj->isdir) {
		die("In directory %u, %s was not a directory",
		    dirnum, name_get(name));
	}
	return de->obj->obj_dir.identity;
}

/*
 * Find a directory's parent by object id, by searching the volume
 * state.
 */
static
unsigned
model_findparent(unsigned dirnum)
{
	struct fsobject *obj;

	obj = finddir(state, dirnum);
	assert(obj->isdir);
	assert(obj->obj_dir.parent->isdir);
	return obj->obj_dir.parent->obj_dir.identity;
}

/*
 * Check if a name (in a specific containing directory) is a file, by
 * searching the volume state.
 */
static
unsigned
model_isfile(unsigned dirnum, unsigned name)
{
	struct fsobject *obj;
	struct fsdirent *de;

	obj = finddir(state, dirnum);
	de = fsdir_find_entry(obj, name, 0);
	return de != NULL && !de->obj->isdir;
}

/*
 * Check if a name (in a specific containing directory) is a
 * directory, by searching the volume state.
 */
static
unsigned
model_isdir(unsigned dirnum, unsigned name)
{
	struct fsobject *obj;
	struct fsdirent *de;

	obj = finddir(state, dirnum);
	de = fsdir_find_entry(obj, name, 0);
	return de != NULL && de->obj->isdir;
}

/*
 * Get the current size of a file in the current volume state.
 */
static
off_t
model_getfilesize(unsigned filenum)
{
	struct fsobject *obj;

	obj = findfile_maybe(state, filenum);
	if (obj == NULL) {
		/* file is unlinked */
		return 0;
	}
	assert(!obj->isdir);
	return obj->obj_file.len;
}

////////////////////////////////////////////////////////////
// model construction (replaying the workload)

/* The workload's current directory. */
static unsigned cwd;

/*
 * Set up for replying the workload: allocate the root directory,
 * generate the newfs record, set up the volume state, initialize
 * the current directory.
 */
void
check_setup(void)
{
	unsigned rootdir;

	assert(firstchange == NULL);
	assert(changes == NULL);
	assert(state == NULL);
	assert(nextfilenum == 0);
	assert(nextdirnum == 0);

	rootdir = nextdirnum++;
	firstchange = changes = fc_create_newfs(rootdir);

	rewindstate();

	/* apply the first change (the newfs record) */
	apply_change(state, changes);

	cwd = rootdir;
}

/*
 * Workload replay: create a file
 */
int
check_createfile(unsigned name)
{
	struct fschange *prevdir;
	unsigned filenum;

	prevdir = changes_findprevdir(cwd);

	filenum = nextfilenum++;
	fc_attach(fc_create_creat(prevdir, cwd, name, filenum));
	return filenum;
}

/*
 * Workload replay: open a file
 */
int
check_openfile(unsigned name)
{
	return model_findfile(cwd, name);
}

/*
 * Workload replay: close a file
 */
void
check_closefile(int handle, unsigned name)
{
	/* don't need to do anything */
	(void)handle;
	(void)name;
}

/*
 * Workload replay: write to a file
 *
 * CODE and SEQ encode the contents to be written.
 */
void
check_write(int handle, unsigned name, unsigned code, unsigned seq,
	    off_t pos, off_t len)
{
	unsigned filenum;
	struct fschange *prevfile;
	off_t prevlen;

	filenum = handle;
	assert(filenum < nextfilenum);
	(void)name;

	prevlen = model_getfilesize(filenum);

	prevfile = changes_findprevfile(filenum);
	fc_attach(fc_create_write(prevfile, filenum, pos, len, prevlen,
				  code, seq));
}

/*
 * Workload replay: truncate a file
 */
void
check_truncate(int handle, unsigned name, off_t len)
{
	unsigned filenum;
	struct fschange *prevfile;

	filenum = handle;
	assert(filenum < nextfilenum);
	(void)name;

	prevfile = changes_findprevfile(filenum);
	fc_attach(fc_create_truncate(prevfile, filenum, len));
}

/*
 * Workload replay: create a directory
 */
void
check_mkdir(unsigned name)
{
	struct fschange *prevdir;
	unsigned dirnum;

	prevdir = changes_findprevdir(cwd);
	dirnum = nextdirnum++;
	fc_attach(fc_create_mkdir(prevdir, cwd, name, dirnum));
}

/*
 * Workload replay: remove a directory
 */
void
check_rmdir(unsigned name)
{
	struct fschange *prevdir, *prevvictim;
	unsigned victim;

	prevdir = changes_findprevdir(cwd);
	victim = model_finddir(cwd, name);
	prevvictim = changes_findprevdir(victim);

	fc_attach(fc_create_rmdir(prevdir, prevvictim, cwd, name, victim));
}

/*
 * Workload replay: remove a file
 */
void
check_unlink(unsigned name)
{
	struct fschange *prevdir, *prevvictim;
	unsigned victim;

	prevdir = changes_findprevdir(cwd);
	victim = model_findfile(cwd, name);
	prevvictim = changes_findprevfile(victim);

	fc_attach(fc_create_unlink(prevdir, prevvictim, cwd, name, victim));
}

/*
 * Workload replay: hard link a file
 */
void
check_link(unsigned fromname, unsigned toname)
{
	struct fschange *prevdir, *prevfile;
	unsigned filenum;

	prevdir = changes_findprevdir(cwd);
	filenum = model_findfile(cwd, fromname);
	prevfile = changes_findprevfile(filenum);

	fc_attach(fc_create_link(prevdir, prevdir, prevfile,
				 cwd, fromname, cwd, toname, filenum));
}

/*
 * Workload replay: rename something
 */
static
void
common_rename(unsigned fromdirnum, unsigned fromname,
	      unsigned todirnum, unsigned toname)
{
	struct fschange *prevfromdir, *prevtodir, *prevfrom, *prevto;
	unsigned fromnum, tonum;
	struct fschange *fc;
	int isfile;

	prevfromdir = changes_findprevdir(fromdirnum);
	prevtodir = changes_findprevdir(todirnum);

	if (model_isfile(todirnum, toname)) {
		isfile = 1;
		assert(model_isfile(fromdirnum, fromname));
		tonum = model_findfile(todirnum, toname);
		prevto = changes_findprevfile(tonum);
		fc = fc_create_unlink(prevtodir, prevto,
				      todirnum, toname, tonum);
	}
	else if (model_isdir(todirnum, toname)) {
		isfile = 0;
		assert(model_isdir(fromdirnum, fromname));
		tonum = model_finddir(todirnum, toname);
		prevto = changes_findprevdir(tonum);
		fc = fc_create_rmdir(prevtodir, prevto,
				     todirnum, toname, tonum);
	}
	else {
		isfile = model_isfile(fromdirnum, fromname);
		fc = NULL;
	}

	if (fc) {
		fc->partial = 1;
		fc_attach(fc);
	}

	if (isfile) {
		fromnum = model_findfile(fromdirnum, fromname);
		prevfrom = changes_findprevfile(fromnum);
		fc = fc_create_renamefile(prevfromdir, prevtodir, prevfrom,
					  fromdirnum, fromname,
					  todirnum, toname, fromnum);
	}
	else {
		fromnum = model_finddir(fromdirnum, fromname);
		prevfrom = changes_findprevdir(fromnum);
		fc = fc_create_renamedir(prevfromdir, prevtodir, prevfrom,
					 fromdirnum, fromname,
					 todirnum, toname, fromnum);
	}
	fc_attach(fc);
}

/*
 * Workload replay: rename within the current directory
 */
void
check_rename(unsigned from, unsigned to)
{
	common_rename(cwd, from, cwd, to);
}

/*
 * Workload replay: cross-directory rename
 */
void
check_renamexd(unsigned fromdir, unsigned from, unsigned todir, unsigned to)
{
	unsigned fromdirnum, todirnum;

	/* fromdir/todir are names in cwd */
	fromdirnum = model_finddir(cwd, fromdir);
	todirnum = model_finddir(cwd, todir);

	common_rename(fromdirnum, from, todirnum, to);
}

/*
 * Workload replay: change directory
 */
void
check_chdir(unsigned name)
{
	cwd = model_finddir(cwd, name);
}

/*
 * Workload replay: change directory to ..
 */
void
check_chdirup(void)
{
	cwd = model_findparent(cwd);
}

/*
 * Workload replay: sync
 */
void
check_sync(void)
{
	/* nothing */
}

////////////////////////////////////////////////////////////
// inspection of the fs

/* the root of the state we found (as opposed to the state we modeled) */
static struct fsobject *found;

/* count of how much we found */
static unsigned found_subdirs, found_files;

/*
 * Wrapper. As of this writing OS/161 provides fstat but not stat...
 * grr
 */
static
int
xstat(const char *path, struct stat *buf)
{
	int fd, ret, serrno;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	ret = fstat(fd, buf);
	serrno = errno;
	close(fd);
	errno = serrno;
	return ret;
}

/*
 * Inspect a directory. PARENTOBJ is the *containing* directory;
 * PARENTINO is its inode number, and DIRNAMESTR is the name of the
 * directory to inspect. The arguments are set up this way (rather
 * than e.g. passing the fsobject of the directory to inspect) to
 * improve the error reporting, and to allow checking .., and so
 * forth.
 */
static
struct fsobject *
inspectdir(struct fsobject *parentobj, ino_t parentino, const char *dirnamestr)
{
	char subnamestr[NAME_MAX+1];
	size_t subnamelen;
	struct stat dirstat, dotstat, substat;
	int dirfd;
	struct fsobject *subobj, *ret;
	struct fsdirent *contents, *de;

	/*
	 * Stat the target, and cd into it.
	 */

	if (xstat(dirnamestr, &dirstat)) {
		err(1, "%s: stat", dirnamestr);
	}

	assert(S_ISDIR(dirstat.st_mode));
	if (chdir(dirnamestr)) {
		err(1, "%s: chdir", dirnamestr);
	}

	/*
	 * Check that . is correct
	 */

	if (xstat(".", &dotstat)) {
		err(1, "In %s: .: stat", dirnamestr);
	}
	if (dotstat.st_dev != dirstat.st_dev) {
		errx(1, "in %s: .: wrong volume id; found %u, expected %u",
		     dirnamestr, dotstat.st_dev, dirstat.st_dev);
	}
	if (dotstat.st_ino != dirstat.st_ino) {
		errx(1, "%s/.: wrong inode number; found %u, expected %u",
		     dirnamestr, dotstat.st_ino, dirstat.st_ino);
	}

	/*
	 * Check that .. leads back
	 */

	if (xstat("..", &dotstat)) {
		err(1, "In %s: ..: stat", dirnamestr);
	}
	if (dotstat.st_dev != dirstat.st_dev) {
		errx(1, "In %s: ..: wrong volume id; found %u, expected %u",
		     dirnamestr, dotstat.st_dev, dirstat.st_dev);
	}
	if (dotstat.st_ino != parentino) {
		errx(1, "In %s: ..: wrong inode number; found %u, expected %u",
		     dirnamestr, dotstat.st_ino, parentino);
	}

	/*
	 * Create a directory fsobject.
	 */

	dirfd = open(".", O_RDONLY);
	if (dirfd < 0) {
		err(1, "In %s: .: open", dirnamestr);
	}

	ret = fsobject_create_dir(UNKNOWN_ID, parentobj);

	/*
	 * Read the contents of the target directory and create
	 * directory entries for them. Recurse for ones that are
	 * themselves directories.
	 */
	contents = NULL;
	while (1) {
		subnamelen = getdirentry(dirfd, subnamestr,
					 sizeof(subnamestr)-1);
		if (subnamelen == 0) {
			break;
		}
		subnamestr[subnamelen] = 0;

		if (!strcmp(subnamestr, ".") || !strcmp(subnamestr, "..")) {
			continue;
		}
		if (xstat(subnamestr, &substat)) {
			err(1, "In %s: %s: stat", dirnamestr, subnamestr);
		}
		if (S_ISDIR(substat.st_mode)) {
			subobj = inspectdir(ret, dirstat.st_ino, subnamestr);
			found_subdirs++;
		}
		else {
			subobj = fsobject_create_file(UNKNOWN_ID);
			subobj->obj_file.len = substat.st_size;
			found_files++;
		}
		de = fsdirent_create(name_find(subnamestr), subobj);
		de->next = contents;
		contents = de;
	}

	/*
	 * Done scanning; cd out, save the contents, and return the
	 * new object.
	 */

	close(dirfd);
	if (chdir("..")) {
		err(1, "In %s; ..: chdir", dirnamestr);
	}

	ret->obj_dir.entries = contents;

	return ret;
}

/*
 * Inspect the whole volume by starting with "." -- we assume that we
 * were run in the root directory.
 */
static
void
inspectfs(void)
{
	struct stat st;

	if (xstat(".", &st)) {
		err(1, ".: stat");
	}
	found = inspectdir(NULL, st.st_ino, ".");
}

////////////////////////////////////////////////////////////
// comparison of state trees

/*
 * Count the number of objects at and below a particular fsobject.
 */
static
unsigned
count_subtree(struct fsobject *obj)
{
	struct fsdirent *de;
	unsigned ret = 1;

	if (obj->isdir) {
		for (de = obj->obj_dir.entries; de != NULL; de = de->next) {
			ret += count_subtree(de->obj);
		}
	}
	return ret;
}

/*
 * Compare two fsobjects. Return the matching score. (lower scores are
 * better matches)
 */
static
unsigned
compare_objects(struct fsobject *obja, struct fsobject *objb)
{
	struct fsdirent *enta, *entb;
	unsigned ret, found;

	if (obja->isdir != objb->isdir) {
		/*
		 * One object's a file, the other's a directory.
		 *
		 * Return one point for each name in the missing
		 * subtree. This includes one point for the top dir,
		 * which is mismatched rather than missing.
		 */
		if (obja->isdir) {
			return count_subtree(obja);
		}
		assert(objb->isdir);
		return count_subtree(objb);
	}

	if (!obja->isdir) {
		/* 
		 * Both objects are files
		 */
		assert(!objb->isdir);
		if (obja->obj_file.len != objb->obj_file.len) {
			/* one point for the size being different */
			return 1;
		}
		return 0;
	}

	/* 
	 * Both objects are directories. Recurse on all pairs of
	 * entries that have the same name. Return one point for each
	 * name that's present (recursively) in one object but not the
	 * other.
	 *
	 * XXX: sort the entries first or something instead of naively
	 * searching 2*N^2 times.
	 */
	assert(obja->isdir);
	assert(objb->isdir);

	ret = 0;
	for (enta = obja->obj_dir.entries; enta != NULL; enta = enta->next) {
		found = 0;
		for (entb = objb->obj_dir.entries; entb != NULL;
		     entb = entb->next) {
			if (enta->name == entb->name) {
				ret += compare_objects(enta->obj, entb->obj);
				found = 1;
				break;
			}
		}
		if (!found) {
			if (enta->obj->isdir) {
				/* whole subtree is missing */
				ret += count_subtree(enta->obj);
			}
			/* one file is missing */
			ret += 1;
		}
	}


	for (entb = objb->obj_dir.entries; entb != NULL; entb = entb->next) {
		found = 0;
		for (enta = obja->obj_dir.entries; enta != NULL;
		     enta = enta->next) {
			if (enta->name == entb->name) {
				found = 1;
				break;
			}
		}
		if (!found) {
			if (entb->obj->isdir) {
				/* whole subtree is missing */
				ret += count_subtree(entb->obj);
			}
			/* one file is missing */
			ret += 1;
		}
	}

	return ret;
}

/*
 * Print an indentation.
 */
static
void
doindent(unsigned depth)
{
	unsigned i;

	for (i=0; i<depth; i++) {
		printf("   ");
	}
}

/*
 * Print out the differences between two fsobjects. Otherwise much the
 * same as compare_objects.
 *
 * OBJA is "expected" (the model); OBJB is "found" (what we saw on disk).
 */
static
void
printdiffs(unsigned indent, struct fsobject *obja, struct fsobject *objb)
{
	struct fsdirent *enta, *entb;
	unsigned found;

	assert(obja->isdir);
	assert(objb->isdir);

	for (enta = obja->obj_dir.entries; enta != NULL; enta = enta->next) {
		found = 0;
		for (entb = objb->obj_dir.entries; entb != NULL;
		     entb = entb->next) {
			if (enta->name == entb->name) {
				doindent(indent);
				printf("%s", name_get(enta->name));
				if (enta->obj->isdir &&
				    !entb->obj->isdir) {
					printf(": expected dir, found file;");
					printf(" %u names missing.\n",
					       count_subtree(enta->obj) - 1);
				}
				else if (!enta->obj->isdir &&
					 entb->obj->isdir) {
					printf(": expected file, found dir;");
					printf(" %u extra names.\n",
					       count_subtree(entb->obj) - 1);
				}
				else if (!enta->obj->isdir &&
					 !entb->obj->isdir) {
					off_t alen, blen;

					alen = enta->obj->obj_file.len;
					blen = entb->obj->obj_file.len;
					if (alen == blen) {
						printf("\t\t%lld bytes (ok)\n",
						       alen);
					}
					else {
						printf(": found %lld bytes, "
						       "expected %lld "
						       "bytes.\n",
						       blen, alen);
					}
				}
				else {
					printf("/\n");
					printdiffs(indent + 1,
						   enta->obj, entb->obj);
				}
				found = 1;
				break;
			}
		}
		if (!found) {
			doindent(indent);
			printf("%s: missing ", name_get(enta->name));
			if (enta->obj->isdir) {
				printf("subtree with %u names.\n",
				       count_subtree(enta->obj) - 1);
			}
			else {
				printf("file\n");
			}
		}
	}


	for (entb = objb->obj_dir.entries; entb != NULL; entb = entb->next) {
		found = 0;
		for (enta = obja->obj_dir.entries; enta != NULL;
		     enta = enta->next) {
			if (enta->name == entb->name) {
				found = 1;
				break;
			}
		}
		if (!found) {
			doindent(indent);
			printf("%s: extra ", name_get(entb->name));
			if (entb->obj->isdir) {
				printf("subtree with %u names.\n",
				       count_subtree(entb->obj) - 1);
			}
			else {
				printf("file\n");
			}
		}
	}
}

////////////////////////////////////////////////////////////
// comparison of file contents

/*
 * Return the version of the oldest change with the same directory
 * structure as CHANGE. This skips past truncate and write operations
 * that only change file sizes.
 */
static
unsigned
findokversion(struct fschange *change)
{
	while (change != NULL) {
		switch (change->type) {
		    case FC_TRUNCATE:
		    case FC_WRITE:
			break;
		    default:
			return change->version;
		}
		change = change->prev;
	}
	assert(0);
	return 0;
}

/*
 * Back up to previous change entries until we find one affecting the
 * contents of the specified file. (That means: truncate, write, or
 * create.)
 *
 * Returns the current (passed-in) change if that matches.
 */
static
struct fschange *
backup_for_file(struct fschange *change, unsigned filenum)
{
	while (change != NULL) {
		switch (change->type) {
		    case FC_TRUNCATE:
			if (change->fc_truncate.file == filenum) {
				return change;
			}
			break;
		    case FC_WRITE:
			if (change->fc_write.file == filenum) {
				return change;
			}
			break;
		    case FC_CREAT:
			if (change->fc_creat.newfile == filenum) {
				return change;
			}
			break;
		    default:
			break;
		}
		change = change->prev;
	}
	return NULL;
}

/*
 * Expect zeros in a file from START to END. The file is given by FD
 * and named NAMESTR. Report if we find stuff other than zeros.
 */
static
void
checkfilezeros(int fd, const char *namestr, off_t start, off_t end)
{
	char buf[1024];
	size_t len, i;
	ssize_t ret;
	unsigned poison = 0, trash = 0;
	off_t origstart = start;

	printf("   %lld - %lld (expecting zeros)\n", start, end);

	if (lseek(fd, start, SEEK_SET) == -1) {
		err(1, "%s: lseek to %lld", namestr, start);
	}
	while (start < end) {
		/* XXX this assumes end - start fits in size_t */
		len = end - start;
		if (len > sizeof(buf)) {
			len = sizeof(buf);
		}
		ret = read(fd, buf, len);
		if (ret == -1) {
			err(1, "%s: read %u at %lld", namestr, len, start);
		}
		if (ret == 0) {
			errx(1, "%s: read %u at %lld: Unexpected EOF",
			     namestr, len, start);
		}
		for (i=0; i<(size_t)ret; i++) {
			if ((unsigned char)buf[i] == POISON_VAL) {
				poison++;
			}
			else if (buf[i] != 0) {
				trash++;
			}
		}
		start += ret;
	}
	if (poison > 0 || trash > 0) {
		printf("ERROR: File %s: expected zeros from %lld to %lld; "
		       "found",
		       namestr, origstart, end);
		if (poison > 0) {
			printf(" %u poison bytes", poison);
			if (trash > 0) {
				printf(" and");
			}
		}
		if (trash > 0) {
			printf(" %u trash bytes", trash);
		}
		printf("\n");
	}
}

/*
 * Read data in from a file, into data.c's read buffer where it will
 * be checked for correctness. The data.c region involved is
 * REGIONSTART..REGIONEND; all of that space is needed so the expected
 * data can be generated correctly. However, we're only looking at the
 * range CHECKSTART..CHECKEND, which may be only part of the data.c
 * region.
 */
static
void
readfiledata(int fd, const char *namestr,
	      off_t regionstart, off_t checkstart,
	      off_t checkend, off_t regionend)
{
	char *readbuf;
	size_t bufpos, len;
	ssize_t ret;

	/* CHECKSTART..CHECKEND must be within REGIONSTART..REGIONEND */
	assert(regionstart <= checkstart);
	assert(checkstart <= checkend);
	assert(checkend <= regionend);

	readbuf = data_mapreadbuf(regionend - regionstart);
	bufpos = checkstart - regionstart;
	len = checkend - checkstart;
	if (lseek(fd, checkstart, SEEK_SET) == -1) {
		err(1, "%s: lseek to %lld", namestr, checkstart);
	}

	while (len > 0) {
		ret = read(fd, readbuf + bufpos, len);
		if (ret == -1) {
			err(1, "%s: read %u at %lld",
			    namestr, len, regionstart + bufpos);
		}
		if (ret == 0) {
			errx(1, "%s: read %u at %lld: Unexpected EOF",
			     namestr, len, regionstart + bufpos);
		}
		bufpos += ret;
		assert(len >= (size_t)ret);
		len -= ret;
	}
}

/*
 * Check the data found in the file FD named NAMESTR, in the range
 * CHECKSTART..CHECKEND, which is within or equal to the data.c
 * region REGIONSTART..REGIONEND. ZEROSTART is the place where the
 * data can legitimately be zeroed rather than the expected data
 * (the portion of a write that extended a file) and the data is
 * generated from CODE and SEQ.
 */
static
void
checkfiledata(int fd, const char *namestr, unsigned code, unsigned seq,
	      off_t zerostart,
	      off_t regionstart, off_t checkstart,
	      off_t checkend, off_t regionend)
{
	if (checkstart < regionstart) {
		checkstart = regionstart;
	}
	if (checkend > regionend) {
		checkend = regionend;
	}

	printf("   %lld - %lld\n", checkstart, checkend);

	readfiledata(fd, namestr,
		     regionstart, checkstart, checkend, regionend);

	data_check(namestr, regionstart,
		   code, seq, zerostart - regionstart, regionend - regionstart,
		   checkstart - regionstart, checkend - checkstart);
}

/*
 * Check a range of the file FD named NAMESTR, from START to END,
 * against the model state expected as of CHANGE.
 */
static
void
checkfilerange(int fd, const char *namestr, struct fschange *change,
	       off_t start, off_t end)
{
	assert(start < end);
	if (change->type == FC_TRUNCATE) {
		off_t tlen;
		struct fschange *prev;

		/*
		 * The most recent change was a truncate. Anything
		 * beyond the truncate length should read as zeroes;
		 * recurse on the part before (if any) using the
		 * previous change affecting this file.
		 *
		 * We might be checking that a chunk of the file
		 * beyond the truncate length is zero (even though as
		 * of this change it's past EOF) if a later change
		 * made that region into a hole in a sparse file.
		 */

		tlen = change->fc_truncate.len;
		prev = change->fc_truncate.prev_thisfile;

		if (tlen < start) {
			checkfilezeros(fd, namestr, start, end);
		}
		else if (tlen < end) {
			checkfilerange(fd, namestr, prev, start, tlen);
			checkfilezeros(fd, namestr, tlen, end);
		}
		else {
			checkfilerange(fd, namestr, prev, start, end);
		}
	}
	else if (change->type == FC_WRITE) {
		off_t wstart, wend;
		struct fschange *prev;
		unsigned code, seq;
		off_t oldfilesize, zerostart;

		/*
		 * The most recent change was a write.
		 */

		wstart = change->fc_write.pos;
		wend = change->fc_write.pos + change->fc_write.len;
		prev = change->fc_write.prev_thisfile;
		code = change->fc_write.code;
		seq = change->fc_write.seq;
		oldfilesize = change->fc_write.oldfilesize;

		/*
		 * ZEROSTART (where we begin to allow the file to
		 * contain zeros) should be the point at the write
		 * where we began to extend the file, if any.
		 */
		if (oldfilesize < wstart) {
			zerostart = wstart;
		}
		else if (oldfilesize < wend) {
			zerostart = oldfilesize;
		}
		else {
			zerostart = wend;
		}

		/*
		 * Six cases for the range overlap:
		 *     (1)    (2)    (3)    (4)   (5)    (6)
		 *    **     ***    ******   *     ***      **
		 *       ***   ***    ***   ***  ***    ***
		 *
		 * We call checkfilerange recursively using the
		 * previous change for this file for the
		 * non-overlapping parts, because this write didn't
		 * touch those so they must reflect the previous file
		 * state; and checkfiledata for the overlapping parts
		 * that this write did touch.
		 */

		if (end <= wstart || start >= wend) {
			/* cases 1 and 6 */
			checkfilerange(fd, namestr, prev, start, end);
		}
		else {
			/* cases 2-5 */
			if (start < wstart) {
				/* case 2 or 3 */
				checkfilerange(fd, namestr, prev,
					       start, wstart);
			}
			checkfiledata(fd, namestr, code, seq, zerostart,
				      wstart, start, end, wend);
			if (end > wend) {
				/* cases 3 or 5 */
				checkfilerange(fd, namestr, prev, wend, end);
			}
		}
	}
	else if (change->type == FC_RENAMEFILE) {
		struct fschange *prev;

		/*
		 * The most recent change was a rename. As rename
		 * doesn't affect contents, recurse on the previous
		 * change affecting the file.
		 */

		prev = change->fc_renamefile.prev_movedfile;
		checkfilerange(fd, namestr, prev, start, end);
	}
	else {
		assert(change->type == FC_CREAT);

		/*
		 * The most recent change was the file being created.
		 * Like with truncate, check that the range is zero
		 * even though it's past EOF, as a later write might
		 * have converted past-EOF space into a hole in a
		 * sparse file.
		 */
		checkfilezeros(fd, namestr, start, end);
	}
}

/*
 * Check if a change CHANGE to a file is present in the observed file
 * FD, whose name is NAMESTR and size is FILESIZE.
 *
 * For writes it checks the data, so should be accurate; for truncate
 * all it can check is the size, so it can readily be fooled by
 * sequences of truncates or multiple truncates to the same length.
 */
static
int
change_is_present(int fd, const char *namestr, off_t filesize,
		  struct fschange *change)
{
	off_t pos, len, oldfilesize, zerostart;
	unsigned code, seq;

	switch (change->type) {
	    case FC_TRUNCATE:
		return filesize == change->fc_truncate.len;
	    case FC_WRITE:
		pos = change->fc_write.pos;
		len = change->fc_write.len;
		code = change->fc_write.code;
		seq = change->fc_write.seq;
		oldfilesize = change->fc_write.oldfilesize;
		if (oldfilesize < pos) {
			zerostart = 0;
		}
		else if (oldfilesize < pos + len) {
			zerostart = oldfilesize - pos;
		}
		else {
			zerostart = len;
		}
		readfiledata(fd, namestr, pos, pos, pos+len, pos+len);
		return data_matches(namestr, pos,
				    code, seq, zerostart, len, 0, len);
	    case FC_CREAT:
		return 1;
	    default:
		break;
	}
	assert(0);
	return 0;
}

/*
 * Check the contents of the file called NAMESTR, which is the model
 * file FILE, as of change CHANGE.
 */
static
void
checkonefilecontents(const char *namestr, struct fsobject *file,
		     struct fschange *change)
{
	unsigned okversion;
	int fd;

	/*
	 * Open the file.
	 */

	assert(!file->isdir);

	fd = open(namestr, O_RDONLY);
	if (fd < 0) {
		err(1, "%s: open", namestr);
	}

	/*
	 * Find the oldest version that has the same directory
	 * structure as CHANGE, and thus reflects the earliest
	 * contents we can legitimately find in the file.
	 *
	 * XXX: the matching we do to pick the change to examine takes
	 * file sizes into account, but findokversion specifically
	 * doesn't stop going backwards if just the file size changes.
	 * This seems wrong: if we match on file size, ok versions to
	 * see are only those that have the same file size.
	 */
	okversion = findokversion(change);

	/*
	 * Find the first change (going backwards) that affects this
	 * file. There should always be at least the create. This
	 * change might be before or after OKVERSION.
	 */
	change = backup_for_file(change, file->obj_file.identity);
	if (change == NULL) {
		die("File %s was never even created?", namestr);
	}

	if (file->obj_file.len == 0) {
		/*
		 * The model expects the length to be 0.
		 *
		 * XXX: I think the code here was written thinking
		 * FILE is from the inspection results; but it's not,
		 * it's the model from the workload replay. So these
		 * checks appear to be wrong.
		 */
		if (change->type == FC_CREAT) {
			/* file was created empty and never written to */
			return;
		}
		if (change->type == FC_TRUNCATE) {
			/* if the length is wrong we shouldn't get this far */
			assert(change->fc_truncate.len == 0);

			/* so, nothing to check */
			close(fd);
			return;
		}
		assert(change->type == FC_WRITE);
		printf("ERROR: File %s is zero length but was expected to "
		       "contain at least %lld bytes at offset %lld!\n",
		       namestr, change->fc_write.pos, change->fc_write.len);
		close(fd);
		return;
	}

	/* XXX: this check is wrong too. */
	if (change->type == FC_CREAT) {
		printf("ERROR: File %s was never written to but has "
		       "length %lld\n",
		       namestr, file->obj_file.len);
		close(fd);
		return;
	}

	/*
	 * If CHANGE isn't reflected in the file, back up. If this
	 * causes us to back up past OKVERSION, complain that the
	 * change should be present.
	 *
	 * XXX: the call to change_is_present should be using the
	 * observed file size, not the modeled file size.
	 *
	 * XXX: More seriously, however, this logic is not really
	 * right. If the workload contains multiple writes that don't
	 * affect the file length, interspersed with truncates that
	 * are all to the same length, we'll stop looking back at the
	 * first truncate (because the length matches) regardless of
	 * which writes are present. None of the workloads currently
	 * does anything like this, but it's still wrong.
	 *
	 * I think the right thing to do (especially since the version
	 * matching checks file lengths) is to check only writes for
	 * presence and ignore truncates; and probably, rather than
	 * back up based on what writes appear to be present and then
	 * call checkfilerange, which will then complain loudly if
	 * parts of an older write didn't make it out, divide the file
	 * into ranges based on blocks and writes and figure out which
	 * version the data found in each such range corresponds to,
	 * then object only to the ones that are crazy and warn about
	 * ones that reflect data buffers not making it out. But this
	 * is a fairly big rewrite.
	 */
	while (!change_is_present(fd, namestr, file->obj_file.len, change)) {
		if (change->version < okversion) {
			printf("File %s: change for version %u is missing\n",
			       namestr, change->version);
		}
		change = backup_for_file(change->prev,file->obj_file.identity);
		if (change == NULL) {
			printf("File %s: no matching version found\n",
			       namestr);
			close(fd);
			return;
		}
	}

	/*
	 * Now we've found a version that we think the file contents
	 * should correspond to; check that it's what we actually
	 * have.
	 *
	 * XXX: should this use the model length or the observed
	 * length?
	 */

	checkfilerange(fd, namestr, change, 0, file->obj_file.len);
	close(fd);
}

/*
 * Check the contents of all files under DIR with respect to the
 * change CHANGE. Recurses on subdirectories.
 */
static
void
checkallfilecontents(struct fsobject *dir, struct fschange *change)
{
	struct fsdirent *de;
	const char *namestr;

	assert(dir->isdir);
	for (de = dir->obj_dir.entries; de != NULL; de = de->next) {
		namestr = name_get(de->name);
		if (de->obj->isdir) {
			printf(" >>> Entering %s\n", namestr);
			if (chdir(namestr)) {
				err(1, "%s: chdir", namestr);
			}
			checkallfilecontents(de->obj, change);
			printf(" <<< Leaving %s\n", namestr);
			if (chdir("..")) {
				err(1, "..: chdir");
			}
		}
		else {
			printf("%s...\n", namestr);
			checkonefilecontents(namestr, de->obj, change);
		}
	}
}

////////////////////////////////////////////////////////////
// model validation

/*
 * Compare the on-disk state we see to the model we've built.
 */
void
checkfs(void)
{
	struct fschange *change, *best;
	unsigned score, bestscore;

	/*
	 * We just built the model; talk about it.
	 */
	printf("Established %u versions across %u directories and %u files\n",
	       changes->version + 1, nextdirnum, nextfilenum);

	/*
	 * Inspect the volume state we've got. Initializes the global
	 * FOUND holding the found volume state.
	 */
	inspectfs();
	printf("Found %u subdirs and %u files on the volume\n",
	       found_subdirs, found_files);

	/*
	 * Rewind the model state to the beginning.
	 */
	rewindstate();

	/*
	 * Loop through all the changes; apply each one to the model
	 * state, and score the model state against the found state.
	 * Keep track of the best matching version.
	 *
	 * XXX: there might be several versions with the same score,
	 * in which case we'll blindly take the most recent one. If
	 * there are several versions that match exactly (which can
	 * easily happen for workloads that don't explicitly avoid it)
	 * we'll also blindly take the most recent one, and then the
	 * matching of file contents can blow up. We should collect
	 * all the versions with the same score, and score each one
	 * on file contents too. Or something like that.
	 */

	change = firstchange;
	assert(change->type == FC_NEWFS);
	best = NULL;
	bestscore = 0;

	while (change != NULL) {
		apply_change(state, change);
		score = compare_objects(state, found);
		if (best == NULL || score <= bestscore) {
			best = change;
			bestscore = score;
		}
		//printf("version %u score %u\n", change->version, score);
		change = change->next;
	}
	assert(best != NULL);

	/*
	 * Set the model state to the best matching state we found.
	 */

	rewindstate();
	advancestateto(best);

	if (bestscore > 0) {
		/*
		 * We didn't get an exact match, so print how the
		 * differences. XXX: this results in not checking file
		 * data...
		 */
		printf("FAILURE: Directory tree does not match on any "
		       "version.\n");
		printf("Best version is %u; describing differences:\n",
		       best->version);
		printdiffs(1, state, found);
		return;
	}

	/*
	 * Ok, we did get an exact match. Print it.
	 */

	printf("Directory tree matched in version %u.\n", best->version);
	if (best->partial) {
		printf("WARNING: this is a version from a partially committed "
		       "operation.\n");
	}

	/*
	 * XXX we should check hard links here; that is, that all the
	 * files that are supposed to be hard links of each other are,
	 * and that no other files are randomly hardlinked.
	 *
	 * However, if things are wrong it should result in bad
	 * contents. It would be very unlikely for even a very broken
	 * filesystem to, on its own, copy files that are supposed to
	 * be hardlinked.
	 */

	/* now check the file contents */

	printf("Checking file contents...\n");
	checkallfilecontents(state, best);
	printf("Done.\n");
}
