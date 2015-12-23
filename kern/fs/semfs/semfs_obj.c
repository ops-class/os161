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

#define SEMFS_INLINE
#include "semfs.h"

////////////////////////////////////////////////////////////
// semfs_sem

/*
 * Constructor for semfs_sem.
 */
struct semfs_sem *
semfs_sem_create(const char *name)
{
	struct semfs_sem *sem;
	char lockname[32];
	char cvname[32];

	snprintf(lockname, sizeof(lockname), "sem:l.%s", name);
	snprintf(cvname, sizeof(cvname), "sem:%s", name);

	sem = kmalloc(sizeof(*sem));
	if (sem == NULL) {
		goto fail_return;
	}
	sem->sems_lock = lock_create(lockname);
	if (sem->sems_lock == NULL) {
		goto fail_sem;
	}
	sem->sems_cv = cv_create(cvname);
	if (sem->sems_cv == NULL) {
		goto fail_lock;
	}
	sem->sems_count = 0;
	sem->sems_hasvnode = false;
	sem->sems_linked = false;
	return sem;

 fail_lock:
	lock_destroy(sem->sems_lock);
 fail_sem:
	kfree(sem);
 fail_return:
	return NULL;
}

/*
 * Destructor for semfs_sem.
 */
void
semfs_sem_destroy(struct semfs_sem *sem)
{
	cv_destroy(sem->sems_cv);
	lock_destroy(sem->sems_lock);
	kfree(sem);
}

/*
 * Helper to insert a semfs_sem into the semaphore table.
 */
int
semfs_sem_insert(struct semfs *semfs, struct semfs_sem *sem, unsigned *ret)
{
	unsigned i, num;

	KASSERT(lock_do_i_hold(semfs->semfs_tablelock));
	num = semfs_semarray_num(semfs->semfs_sems);
	if (num == SEMFS_ROOTDIR) {
		/* Too many */
		return ENOSPC;
	}
	for (i=0; i<num; i++) {
		if (semfs_semarray_get(semfs->semfs_sems, i) == NULL) {
			semfs_semarray_set(semfs->semfs_sems, i, sem);
			*ret = i;
			return 0;
		}
	}
	return semfs_semarray_add(semfs->semfs_sems, sem, ret);
}

////////////////////////////////////////////////////////////
// semfs_direntry

/*
 * Constructor for semfs_direntry.
 */
struct semfs_direntry *
semfs_direntry_create(const char *name, unsigned semnum)
{
	struct semfs_direntry *dent;

	dent = kmalloc(sizeof(*dent));
	if (dent == NULL) {
		return NULL;
	}
	dent->semd_name = kstrdup(name);
	if (dent->semd_name == NULL) {
		kfree(dent);
		return NULL;
	}
	dent->semd_semnum = semnum;
	return dent;
}

/*
 * Destructor for semfs_direntry.
 */
void
semfs_direntry_destroy(struct semfs_direntry *dent)
{
	kfree(dent->semd_name);
	kfree(dent);
}
