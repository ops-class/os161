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
 * Test code for kmalloc.
 */
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <vm.h> /* for PAGE_SIZE */
#include <test.h>

#include "opt-dumbvm.h"

////////////////////////////////////////////////////////////
// km1/km2

/*
 * Test kmalloc; allocate ITEMSIZE bytes NTRIES times, freeing
 * somewhat later.
 *
 * The total of ITEMSIZE * NTRIES is intended to exceed the size of
 * available memory.
 *
 * kmallocstress does the same thing, but from NTHREADS different
 * threads at once.
 */

#define NTRIES   1200
#define ITEMSIZE  997
#define NTHREADS  8

static
void
kmallocthread(void *sm, unsigned long num)
{
	struct semaphore *sem = sm;
	void *ptr;
	void *oldptr=NULL;
	void *oldptr2=NULL;
	int i;

	for (i=0; i<NTRIES; i++) {
		ptr = kmalloc(ITEMSIZE);
		if (ptr==NULL) {
			if (sem) {
				kprintf("thread %lu: kmalloc returned NULL\n",
					num);
				goto done;
			}
			kprintf("kmalloc returned null; test failed.\n");
			goto done;
		}
		if (oldptr2) {
			kfree(oldptr2);
		}
		oldptr2 = oldptr;
		oldptr = ptr;
	}
done:
	if (oldptr2) {
		kfree(oldptr2);
	}
	if (oldptr) {
		kfree(oldptr);
	}
	if (sem) {
		V(sem);
	}
}

int
kmalloctest(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	kprintf("Starting kmalloc test...\n");
	kmallocthread(NULL, 0);
	kprintf("kmalloc test done\n");

	return 0;
}

int
kmallocstress(int nargs, char **args)
{
	struct semaphore *sem;
	int i, result;

	(void)nargs;
	(void)args;

	sem = sem_create("kmallocstress", 0);
	if (sem == NULL) {
		panic("kmallocstress: sem_create failed\n");
	}

	kprintf("Starting kmalloc stress test...\n");

	for (i=0; i<NTHREADS; i++) {
		result = thread_fork("kmallocstress", NULL,
				     kmallocthread, sem, i);
		if (result) {
			panic("kmallocstress: thread_fork failed: %s\n",
			      strerror(result));
		}
	}

	for (i=0; i<NTHREADS; i++) {
		P(sem);
	}

	sem_destroy(sem);
	kprintf("kmalloc stress test done\n");

	return 0;
}

////////////////////////////////////////////////////////////
// km3

/*
 * Larger kmalloc test. Or at least, potentially larger. The size is
 * an argument.
 *
 * The argument specifies the number of objects to allocate; the size
 * of each allocation rotates through sizes[]. (FUTURE: should there
 * be a mode that allocates random sizes?) In order to hold the
 * pointers returned by kmalloc we first allocate a two-level radix
 * tree whose lower tier is made up of blocks of size PAGE_SIZE/4.
 * (This is so they all go to the subpage allocator rather than being
 * whole-page allocations.)
 *
 * Since PAGE_SIZE is commonly 4096, each of these blocks holds 1024
 * pointers (on a 32-bit machine) or 512 (on a 64-bit machine) and so
 * we can store considerably more pointers than we have memory for
 * before the upper tier becomes a whole page or otherwise gets
 * uncomfortably large.
 *
 * Having set this up, the test just allocates and then frees all the
 * pointers in order, setting and checking the contents.
 */
int
kmalloctest3(int nargs, char **args)
{
#define NUM_KM3_SIZES 5
	static const unsigned sizes[NUM_KM3_SIZES] = { 32, 41, 109, 86, 9 };
	unsigned numptrs;
	size_t ptrspace;
	size_t blocksize;
	unsigned numptrblocks;
	void ***ptrblocks;
	unsigned curblock, curpos, cursizeindex, cursize;
	size_t totalsize;
	unsigned i, j;
	unsigned char *ptr;

	if (nargs != 2) {
		kprintf("kmalloctest3: usage: km3 numobjects\n");
		return EINVAL;
	}

	/* Figure out how many pointers we'll get and the space they need. */
	numptrs = atoi(args[1]);
	ptrspace = numptrs * sizeof(void *);

	/* Figure out how many blocks in the lower tier. */
	blocksize = PAGE_SIZE / 4;
	numptrblocks = DIVROUNDUP(ptrspace, blocksize);

	kprintf("kmalloctest3: %u objects, %u pointer blocks\n",
		numptrs, numptrblocks);

	/* Allocate the upper tier. */
	ptrblocks = kmalloc(numptrblocks * sizeof(ptrblocks[0]));
	if (ptrblocks == NULL) {
		panic("kmalloctest3: failed on pointer block array\n");
	}
	/* Allocate the lower tier. */
	for (i=0; i<numptrblocks; i++) {
		ptrblocks[i] = kmalloc(blocksize);
		if (ptrblocks[i] == NULL) {
			panic("kmalloctest3: failed on pointer block %u\n", i);
		}
	}

	/* Allocate the objects. */
	curblock = 0;
	curpos = 0;
	cursizeindex = 0;
	totalsize = 0;
	for (i=0; i<numptrs; i++) {
		cursize = sizes[cursizeindex];
		ptr = kmalloc(cursize);
		if (ptr == NULL) {
			kprintf("kmalloctest3: failed on object %u size %u\n",
				i, cursize);
			kprintf("kmalloctest3: pos %u in pointer block %u\n",
				curpos, curblock);
			kprintf("kmalloctest3: total so far %zu\n", totalsize);
			panic("kmalloctest3: failed.\n");
		}
		/* Fill the object with its number. */
		for (j=0; j<cursize; j++) {
			ptr[j] = (unsigned char) i;
		}
		/* Move to the next slot in the tree. */
		ptrblocks[curblock][curpos] = ptr;
		curpos++;
		if (curpos >= blocksize / sizeof(void *)) {
			curblock++;
			curpos = 0;
		}
		/* Update the running total, and rotate the size. */
		totalsize += cursize;
		cursizeindex = (cursizeindex + 1) % NUM_KM3_SIZES;
	}

	kprintf("kmalloctest3: %zu bytes allocated\n", totalsize);

	/* Free the objects. */
	curblock = 0;
	curpos = 0;
	cursizeindex = 0;
	for (i=0; i<numptrs; i++) {
		cursize = sizes[cursizeindex];
		ptr = ptrblocks[curblock][curpos];
		KASSERT(ptr != NULL);
		for (j=0; j<cursize; j++) {
			if (ptr[j] == (unsigned char) i) {
				continue;
			}
			kprintf("kmalloctest3: failed on object %u size %u\n",
				i, cursize);
			kprintf("kmalloctest3: pos %u in pointer block %u\n",
				curpos, curblock);
			kprintf("kmalloctest3: at object offset %u\n", j);
			kprintf("kmalloctest3: expected 0x%x, found 0x%x\n",
				ptr[j], (unsigned char) i);
			panic("kmalloctest3: failed.\n");
		}
		kfree(ptr);
		curpos++;
		if (curpos >= blocksize / sizeof(void *)) {
			curblock++;
			curpos = 0;
		}
		KASSERT(totalsize > 0);
		totalsize -= cursize;
		cursizeindex = (cursizeindex + 1) % NUM_KM3_SIZES;
	}
	KASSERT(totalsize == 0);

	/* Free the lower tier. */
	for (i=0; i<numptrblocks; i++) {
		KASSERT(ptrblocks[i] != NULL);
		kfree(ptrblocks[i]);
	}
	/* Free the upper tier. */
	kfree(ptrblocks);

	kprintf("kmalloctest3: passed\n");
	return 0;
}

////////////////////////////////////////////////////////////
// km4

static
void
kmalloctest4thread(void *sm, unsigned long num)
{
#define NUM_KM4_SIZES 5
	static const unsigned sizes[NUM_KM4_SIZES] = { 1, 3, 5, 2, 4 };

	struct semaphore *sem = sm;
	void *ptrs[NUM_KM4_SIZES];
	unsigned p, q;
	unsigned i;

	for (i=0; i<NUM_KM4_SIZES; i++) {
		ptrs[i] = NULL;
	}
	p = 0;
	q = NUM_KM4_SIZES / 2;

	for (i=0; i<NTRIES; i++) {
		if (ptrs[q] != NULL) {
			kfree(ptrs[q]);
			ptrs[q] = NULL;
		}
		ptrs[p] = kmalloc(sizes[p] * PAGE_SIZE);
		if (ptrs[p] == NULL) {
			panic("kmalloctest4: thread %lu: "
			      "allocating %u pages failed\n",
			      num, sizes[p]);
		}
		p = (p + 1) % NUM_KM4_SIZES;
		q = (q + 1) % NUM_KM4_SIZES;
	}

	for (i=0; i<NUM_KM4_SIZES; i++) {
		if (ptrs[i] != NULL) {
			kfree(ptrs[i]);
		}
	}

	V(sem);
}

int
kmalloctest4(int nargs, char **args)
{
	struct semaphore *sem;
	unsigned nthreads;
	unsigned i;
	int result;

	(void)nargs;
	(void)args;

	kprintf("Starting multipage kmalloc test...\n");
#if OPT_DUMBVM
	kprintf("(This test will not work with dumbvm)\n");
#endif

	sem = sem_create("kmalloctest4", 0);
	if (sem == NULL) {
		panic("kmalloctest4: sem_create failed\n");
	}

	/* use 6 instead of 8 threads */
	nthreads = (3*NTHREADS)/4;

	for (i=0; i<nthreads; i++) {
		result = thread_fork("kmalloctest4", NULL,
				     kmalloctest4thread, sem, i);
		if (result) {
			panic("kmallocstress: thread_fork failed: %s\n",
			      strerror(result));
		}
	}

	for (i=0; i<nthreads; i++) {
		P(sem);
	}

	sem_destroy(sem);
	kprintf("Multipage kmalloc test done\n");
	return 0;
}
