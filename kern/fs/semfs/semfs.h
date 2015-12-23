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

#ifndef SEMFS_H
#define SEMFS_H

#include <array.h>
#include <fs.h>
#include <vnode.h>

#ifndef SEMFS_INLINE
#define SEMFS_INLINE INLINE
#endif

/*
 * Constants
 */

#define SEMFS_ROOTDIR	0xffffffffU		/* semnum for root dir */

/*
 * A user-facing semaphore.
 *
 * We don't use the kernel-level semaphore to implement it (although
 * that would be tidy) because we'd have to violate its abstraction.
 * XXX: or would we? review once all this is done.
 */
struct semfs_sem {
	struct lock *sems_lock;			/* Lock to protect count */
	struct cv *sems_cv;			/* CV to wait */
	unsigned sems_count;			/* Semaphore count */
	bool sems_hasvnode;			/* The vnode exists */
	bool sems_linked;			/* In the directory */
};
DECLARRAY(semfs_sem, SEMFS_INLINE);

/*
 * Directory entry; name and reference to a semaphore.
 */
struct semfs_direntry {
	char *semd_name;			/* Name */
	unsigned semd_semnum;			/* Which semaphore */
};
DECLARRAY(semfs_direntry, SEMFS_INLINE);

/*
 * Vnode. These are separate from the semaphore structures so they can
 * come and go at the whim of VOP_RECLAIM. (It might seem tidier to
 * ignore VOP_RECLAIM and destroy vnodes only when the underlying
 * objects are removed; but it ends up being more complicated in
 * practice. XXX: review after finishing)
 */
struct semfs_vnode {
	struct vnode semv_absvn;		/* Abstract vnode */
	struct semfs *semv_semfs;		/* Back-pointer to fs */
	unsigned semv_semnum;			/* Which semaphore */
};

/*
 * The structure for the semaphore file system. Ordinarily there
 * is only one of these.
 */
struct semfs {
	struct fs semfs_absfs;			/* Abstract fs object */

	struct lock *semfs_tablelock;		/* Lock for following */
	struct vnodearray *semfs_vnodes;	/* Currently extant vnodes */
	struct semfs_semarray *semfs_sems;	/* Semaphores */

	struct lock *semfs_dirlock;		/* Lock for following */
	struct semfs_direntryarray *semfs_dents; /* The root directory */
};

/*
 * Arrays
 */

DEFARRAY(semfs_sem, SEMFS_INLINE);
DEFARRAY(semfs_direntry, SEMFS_INLINE);


/*
 * Functions.
 */

/* in semfs_obj.c */
struct semfs_sem *semfs_sem_create(const char *name);
int semfs_sem_insert(struct semfs *, struct semfs_sem *, unsigned *);
void semfs_sem_destroy(struct semfs_sem *);
struct semfs_direntry *semfs_direntry_create(const char *name, unsigned semno);
void semfs_direntry_destroy(struct semfs_direntry *);

/* in semfs_vnops.c */
int semfs_getvnode(struct semfs *, unsigned, struct vnode **ret);


#endif /* SEMFS_H */
