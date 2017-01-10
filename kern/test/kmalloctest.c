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
#include <cpu.h>
#include <thread.h>
#include <synch.h>
#include <vm.h> /* for PAGE_SIZE */
#include <test.h>
#include <kern/test161.h>
#include <mainbus.h>

#include "opt-dumbvm.h"

// from arch/mips/vm/ram.c
extern vaddr_t firstfree;

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

#define PROGRESS(iter) do { \
	if ((iter % 100) == 0) { \
		kprintf("."); \
	} \
} while (0)

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
		PROGRESS(i);
		ptr = kmalloc(ITEMSIZE);
		if (ptr==NULL) {
			if (sem) {
				kprintf("thread %lu: kmalloc returned NULL\n",
					num);
				panic("kmalloc test failed");
			}
			kprintf("kmalloc returned null; test failed.\n");
			panic("kmalloc test failed");
		}
		if (oldptr2) {
			kfree(oldptr2);
		}
		oldptr2 = oldptr;
		oldptr = ptr;
	}

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
	kprintf("\n");
	success(TEST161_SUCCESS, SECRET, "km1");

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
	kprintf("\n");
	success(TEST161_SUCCESS, SECRET, "km2");

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
		PROGRESS(i);
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
		PROGRESS(i);
		KASSERT(ptrblocks[i] != NULL);
		kfree(ptrblocks[i]);
	}
	/* Free the upper tier. */
	kfree(ptrblocks);

	kprintf("\n");
	success(TEST161_SUCCESS, SECRET, "km3");
	return 0;
}

////////////////////////////////////////////////////////////
// km4

static
void
kmalloctest4thread(void *sm, unsigned long num)
{
#define NUM_KM4_SIZES 5
#define ITERATIONS 50
	static const unsigned sizes[NUM_KM4_SIZES] = { 1, 3, 5, 2, 4 };

	struct semaphore *sem = sm;
	void *ptrs[NUM_KM4_SIZES];
	unsigned p, q;
	unsigned i, j, k;
	uint32_t magic;

	for (i=0; i<NUM_KM4_SIZES; i++) {
		ptrs[i] = NULL;
	}
	p = 0;
	q = NUM_KM4_SIZES / 2;
	magic = random();

	for (i=0; i<NTRIES; i++) {
		PROGRESS(i);
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

		// Write to each page of the allocated memory and make sure nothing
		// overwrites it.
		for (k = 0; k < sizes[p]; k++) {
			*((uint32_t *)ptrs[p] + k*PAGE_SIZE/sizeof(uint32_t)) = magic;
		}

		for (j = 0; j < ITERATIONS; j++) {
			random_yielder(4);
			for (k = 0; k < sizes[p]; k++) {
				uint32_t actual = *((uint32_t *)ptrs[p] + k*PAGE_SIZE/sizeof(uint32_t));
				if (actual != magic) {
					panic("km4: expected %u got %u. Your VM is broken!",
						magic, actual);
				}
			}
		}
		magic++;
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

	kprintf("\n");
	success(TEST161_SUCCESS, SECRET, "km4");
	return 0;
}

static inline
void
km5_usage()
{
	kprintf("usage: km5 [--avail <num_pages>] [--kernel <num_pages>]\n");
}

/*
 * Allocate and free all physical memory a number of times. Along the we, we
 * check coremap_used_bytes to make sure it's reporting the number we're
 * expecting.
 */
int
kmalloctest5(int nargs, char **args)
{
#define KM5_ITERATIONS 5

	// We're expecting an even number of arguments, less arg[0].
	if (nargs > 5 || (nargs % 2) == 0) {
		km5_usage();
		return 0;
	}

	unsigned avail_page_slack = 0, kernel_page_limit = 0;
	int arg = 1;

	while (arg < nargs) {
		if (strcmp(args[arg], "--avail") == 0) {
			arg++;
			avail_page_slack = atoi(args[arg++]);
		} else if (strcmp(args[arg], "--kernel") == 0) {
			arg++;
			kernel_page_limit = atoi(args[arg++]);
		} else {
			km5_usage();
			return 0;
		}
	}

#if OPT_DUMBVM
	kprintf("(This test will not work with dumbvm)\n");
#endif

	// First, we need to figure out how much memory we're running with and how
	// much space it will take up if we maintain a pointer to each allocated
	// page. We do something similar to km3 - for 32 bit systems with
	// PAGE_SIZE == 4096, we can store 1024 pointers on a page. We keep an array
	// of page size blocks of pointers which in total can hold enough pointers
	// for each page of available physical memory.
	unsigned orig_used, ptrs_per_page, num_ptr_blocks, max_pages;
	unsigned total_ram, avail_ram, magic, orig_magic, known_pages;

	ptrs_per_page = PAGE_SIZE / sizeof(void *);
	total_ram = mainbus_ramsize();
	avail_ram = total_ram - (uint32_t)(firstfree - MIPS_KSEG0);
	max_pages = (avail_ram + PAGE_SIZE-1) / PAGE_SIZE;
	num_ptr_blocks = (max_pages + ptrs_per_page-1) / ptrs_per_page;

	// The array can go on the stack, we won't have that many
	// (sys161 16M max => 4 blocks)
	void **ptrs[num_ptr_blocks];

	for (unsigned i = 0; i < num_ptr_blocks; i++) {
		ptrs[i] = kmalloc(PAGE_SIZE);
		if (ptrs[i] == NULL) {
			panic("Can't allocate ptr page!");
		}
		bzero(ptrs[i], PAGE_SIZE);
	}

	kprintf("km5 --> phys ram: %uk avail ram: %uk  (%u pages) ptr blocks: %u\n", total_ram/1024,
		avail_ram/1024, max_pages, num_ptr_blocks);

	// Initially, there must be at least 1 page allocated for each thread stack,
	// one page for kmalloc for this thread struct, plus what we just allocated).
	// This probably isn't the GLB, but its a decent lower bound.
	orig_used = coremap_used_bytes();
	known_pages = num_cpus + num_ptr_blocks + 1;
	if (orig_used < known_pages * PAGE_SIZE) {
		panic ("Not enough pages initially allocated");
	}
	if ((orig_used % PAGE_SIZE) != 0) {
		panic("Coremap used bytes should be a multiple of PAGE_SIZE");
	}

	// Test for kernel bloat.
	if (kernel_page_limit > 0) {
		uint32_t kpages = (total_ram - avail_ram + PAGE_SIZE) / PAGE_SIZE;
		if (kpages > kernel_page_limit) {
			panic("You're kernel is bloated! Max allowed pages: %d, used pages: %d",
				kernel_page_limit, kpages);
		}
	}

	orig_magic = magic = random();

	for (int i = 0; i < KM5_ITERATIONS; i++) {
		// Step 1: allocate all physical memory, with checks along the way
		unsigned int block, pos, oom, pages, used, prev;
		void *page;

		block = pos = oom = pages = used = 0;
		prev = orig_used;

		while (pages < max_pages+1) {
			PROGRESS(pages);
			page = kmalloc(PAGE_SIZE);
			if (page == NULL) {
				oom = 1;
				break;
			}

			// Make sure we can write to the page
			*(uint32_t *)page = magic++;

			// Make sure the number of used bytes is going up, and by increments of PAGE_SIZE
			used = coremap_used_bytes();
			if (used != prev + PAGE_SIZE) {
				panic("Allocation not equal to PAGE_SIZE. prev: %u used: %u", prev, used);
			}
			prev = used;

			ptrs[block][pos] = page;
			pos++;
			if (pos >= ptrs_per_page) {
				pos = 0;
				block++;
			}
			pages++;
		}

		// Step 2: Check that we were able to allocate a reasonable number of pages
		unsigned expected;
		if (avail_page_slack > 0 ) {
			// max avail pages + what we can prove we allocated + some slack
			expected = max_pages - (known_pages + avail_page_slack);
		} else {
			// At the very least, just so we know things are working.
			expected = 3;
		}

		if (pages < expected) {
			panic("Expected to allocate at least %d pages, only allocated %d",
				expected, pages);
		}

		// We tried to allocate 1 more page than is available in physical memory. That
		// should fail unless you're swapping out kernel pages, which you should
		// probably not be doing.
		if (!oom) {
			panic("Allocated more pages than physical memory. Are you swapping kernel pages?");
		}

		// Step 3: free everything and check that we're back to where we started
		for (block = 0; block < num_ptr_blocks; block++) {
			for (pos = 0; pos < ptrs_per_page; pos++) {
				if (ptrs[block][pos] != NULL) {
					// Make sure we got unique addresses
					if ((*(uint32_t *)ptrs[block][pos]) != orig_magic++) {
						panic("km5: expected %u got %u - your VM is broken!",
							orig_magic-1, (*(uint32_t *)ptrs[block][pos]));
					}
					kfree(ptrs[block][pos]);
				}
			}
		}

		// Check that we're back to where we started
		used = coremap_used_bytes();
		if (used != orig_used) {
			panic("orig (%u) != used (%u)", orig_used, used);
		}
	}

	//Clean up the pointer blocks
	for (unsigned i = 0; i < num_ptr_blocks; i++) {
		kfree(ptrs[i]);
	}

	kprintf("\n");
	success(TEST161_SUCCESS, SECRET, "km5");

	return 0;
}
