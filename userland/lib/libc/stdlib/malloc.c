/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009, 2014
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
 * User-level malloc and free implementation.
 *
 * This is a basic first-fit allocator. It's intended to be simple and
 * easy to follow. It performs abysmally if the heap becomes larger than
 * physical memory. To get (much) better out-of-core performance, port
 * the kernel's malloc. :-)
 */

#include <stdlib.h>
#include <stdint.h>  // for uintptr_t on non-OS/161 platforms
#include <unistd.h>
#include <err.h>
#include <assert.h>

#undef MALLOCDEBUG

#if defined(__mips__) || defined(__i386__)
#define MALLOC32
#elif defined(__alpha__) || defined(__x86_64__)
#define MALLOC64
#else
#error "please fix me"
#endif

/*
 * malloc block header.
 *
 * mh_prevblock is the downwards offset to the previous header, 0 if this
 * is the bottom of the heap.
 *
 * mh_nextblock is the upwards offset to the next header.
 *
 * mh_pad is unused.
 * mh_inuse is 1 if the block is in use, 0 if it is free.
 * mh_magic* should always be a fixed value.
 *
 * MBLOCKSIZE should equal sizeof(struct mheader) and be a power of 2.
 * MBLOCKSHIFT is the log base 2 of MBLOCKSIZE.
 * MMAGIC is the value for mh_magic*.
 */
struct mheader {

#if defined(MALLOC32)
#define MBLOCKSIZE 8
#define MBLOCKSHIFT 3
#define MMAGIC 2
	/*
	 * 32-bit platform. size_t is 32 bits (4 bytes).
	 * Block size is 8 bytes.
	 */
	unsigned mh_prevblock:29;
	unsigned mh_pad:1;
	unsigned mh_magic1:2;

	unsigned mh_nextblock:29;
	unsigned mh_inuse:1;
	unsigned mh_magic2:2;

#elif defined(MALLOC64)
#define MBLOCKSIZE 16
#define MBLOCKSHIFT 4
#define MMAGIC 6
	/*
	 * 64-bit platform. size_t is 64 bits (8 bytes)
	 * Block size is 16 bytes.
	 */
	unsigned mh_prevblock:60;
	unsigned mh_pad:1;
	unsigned mh_magic1:3;

	unsigned mh_nextblock:60;
	unsigned mh_inuse:1;
	unsigned mh_magic2:3;

#else
#error "please fix me"
#endif
};

/*
 * Operator macros on struct mheader.
 *
 * M_NEXT/PREVOFF:	return offset to next/previous header
 * M_NEXT/PREV:		return next/previous header
 *
 * M_DATA:		return data pointer of a header
 * M_SIZE:		return data size of a header
 *
 * M_OK:		true if the magic values are correct
 *
 * M_MKFIELD:		prepare a value for mh_next/prevblock.
 * 			(value should include the header size)
 */

#define M_NEXTOFF(mh)	((size_t)(((size_t)((mh)->mh_nextblock))<<MBLOCKSHIFT))
#define M_PREVOFF(mh)	((size_t)(((size_t)((mh)->mh_prevblock))<<MBLOCKSHIFT))
#define M_NEXT(mh)	((struct mheader *)(((char*)(mh))+M_NEXTOFF(mh)))
#define M_PREV(mh)	((struct mheader *)(((char*)(mh))-M_PREVOFF(mh)))

#define M_DATA(mh)	((void *)((mh)+1))
#define M_SIZE(mh)	(M_NEXTOFF(mh)-MBLOCKSIZE)

#define M_OK(mh)	((mh)->mh_magic1==MMAGIC && (mh)->mh_magic2==MMAGIC)

#define M_MKFIELD(off)	((off)>>MBLOCKSHIFT)

/*
 * System page size. In POSIX you're supposed to call
 * sysconf(_SC_PAGESIZE). If _SC_PAGESIZE isn't defined, as on OS/161,
 * assume 4K.
 */

#ifdef _SC_PAGESIZE
static size_t __malloc_pagesize;
#define PAGE_SIZE __malloc_pagesize
#else
#define PAGE_SIZE 4096
#endif

////////////////////////////////////////////////////////////

/*
 * Static variables - the bottom and top addresses of the heap.
 */
static uintptr_t __heapbase, __heaptop;

/*
 * Setup function.
 */
static
void
__malloc_init(void)
{
	void *x;

	/*
	 * Check various assumed properties of the sizes.
	 */
	if (sizeof(struct mheader) != MBLOCKSIZE) {
		errx(1, "malloc: Internal error - MBLOCKSIZE wrong");
	}
	if ((MBLOCKSIZE & (MBLOCKSIZE-1))!=0) {
		errx(1, "malloc: Internal error - MBLOCKSIZE not power of 2");
	}
	if (1<<MBLOCKSHIFT != MBLOCKSIZE) {
		errx(1, "malloc: Internal error - MBLOCKSHIFT wrong");
	}

	/* init should only be called once. */
	if (__heapbase!=0 || __heaptop!=0) {
		errx(1, "malloc: Internal error - bad init call");
	}

	/* Get the page size, if needed. */
#ifdef _SC_PAGESIZE
	__malloc_pagesize = sysconf(_SC_PAGESIZE);
#endif

	/* Use sbrk to find the base of the heap. */
	x = sbrk(0);
	if (x==(void *)-1) {
		err(1, "malloc: initial sbrk failed");
	}
	if (x==(void *) 0) {
		errx(1, "malloc: Internal error - heap began at 0");
	}
	__heapbase = __heaptop = (uintptr_t)x;

	/*
	 * Make sure the heap base is aligned the way we want it.
	 * (On OS/161, it will begin on a page boundary. But on
	 * an arbitrary Unix, it may not be, as traditionally it
	 * begins at _end.)
	 */

	if (__heapbase % MBLOCKSIZE != 0) {
		size_t adjust = MBLOCKSIZE - (__heapbase % MBLOCKSIZE);
		x = sbrk(adjust);
		if (x==(void *)-1) {
			err(1, "malloc: sbrk failed aligning heap base");
		}
		if ((uintptr_t)x != __heapbase) {
			err(1, "malloc: heap base moved during init");
		}
#ifdef MALLOCDEBUG
		warnx("malloc: adjusted heap base upwards by %lu bytes",
		      (unsigned long) adjust);
#endif
		__heapbase += adjust;
		__heaptop = __heapbase;
	}
}

////////////////////////////////////////////////////////////

#ifdef MALLOCDEBUG

/*
 * Debugging print function to iterate and dump the entire heap.
 */
static
void
__malloc_dump(void)
{
	struct mheader *mh;
	uintptr_t i;
	size_t rightprevblock;

	warnx("heap: ************************************************");

	rightprevblock = 0;
	for (i=__heapbase; i<__heaptop; i += M_NEXTOFF(mh)) {
		mh = (struct mheader *) i;
		if (!M_OK(mh)) {
			errx(1, "malloc: Heap corrupt; header at 0x%lx"
			     " has bad magic bits",
			     (unsigned long) i);
		}
		if (mh->mh_prevblock != rightprevblock) {
			errx(1, "malloc: Heap corrupt; header at 0x%lx"
			     " has bad previous-block size %lu "
			     "(should be %lu)",
			     (unsigned long) i,
			     (unsigned long) mh->mh_prevblock << MBLOCKSHIFT,
			     (unsigned long) rightprevblock << MBLOCKSHIFT);
		}
		rightprevblock = mh->mh_nextblock;

		warnx("heap: 0x%lx 0x%-6lx (next: 0x%lx) %s",
		      (unsigned long) i + MBLOCKSIZE,
		      (unsigned long) M_SIZE(mh),
		      (unsigned long) (i+M_NEXTOFF(mh)),
		      mh->mh_inuse ? "INUSE" : "FREE");
	}
	if (i!=__heaptop) {
		errx(1, "malloc: Heap corrupt; ran off end");
	}

	warnx("heap: ************************************************");
}

#endif /* MALLOCDEBUG */

////////////////////////////////////////////////////////////

/*
 * Get more memory (at the top of the heap) using sbrk, and
 * return a pointer to it.
 */
static
void *
__malloc_sbrk(size_t size)
{
	void *x;

	x = sbrk(size);
	if (x == (void *)-1) {
		return NULL;
	}

	if ((uintptr_t)x != __heaptop) {
		errx(1, "malloc: Internal error - "
		     "heap top moved itself from 0x%lx to 0x%lx",
		     (unsigned long) __heaptop,
		     (unsigned long) (uintptr_t) x);
	}
	__heaptop += size;
	return x;
}

/*
 * Make a new (free) block from the block passed in, leaving size
 * bytes for data in the current block. size must be a multiple of
 * MBLOCKSIZE.
 *
 * Only split if the excess space is at least twice the blocksize -
 * one blocksize to hold a header and one for data.
 */
static
void
__malloc_split(struct mheader *mh, size_t size)
{
	struct mheader *mhnext, *mhnew;
	size_t oldsize;

	if (size % MBLOCKSIZE != 0) {
		errx(1, "malloc: Internal error (size %lu passed to split)",
		     (unsigned long) size);
	}

	if (M_SIZE(mh) - size < 2*MBLOCKSIZE) {
		/* no room */
		return;
	}

	mhnext = M_NEXT(mh);

	oldsize = M_SIZE(mh);
	mh->mh_nextblock = M_MKFIELD(size + MBLOCKSIZE);

	mhnew = M_NEXT(mh);
	if (mhnew==mhnext) {
		errx(1, "malloc: Internal error (split screwed up?)");
	}

	mhnew->mh_prevblock = M_MKFIELD(size + MBLOCKSIZE);
	mhnew->mh_pad = 0;
	mhnew->mh_magic1 = MMAGIC;
	mhnew->mh_nextblock = M_MKFIELD(oldsize - size);
	mhnew->mh_inuse = 0;
	mhnew->mh_magic2 = MMAGIC;

	if (mhnext != (struct mheader *) __heaptop) {
		mhnext->mh_prevblock = mhnew->mh_nextblock;
	}
}

/*
 * malloc itself.
 */
void *
malloc(size_t size)
{
	struct mheader *mh;
	uintptr_t i;
	size_t rightprevblock;
	size_t morespace;
	void *p;

	if (__heapbase==0) {
		__malloc_init();
	}
	if (__heapbase==0 || __heaptop==0 || __heapbase > __heaptop) {
		warnx("malloc: Internal error - local data corrupt");
		errx(1, "malloc: heapbase 0x%lx; heaptop 0x%lx",
		     (unsigned long) __heapbase, (unsigned long) __heaptop);
	}

#ifdef MALLOCDEBUG
	warnx("malloc: about to allocate %lu (0x%lx) bytes",
	      (unsigned long) size, (unsigned long) size);
	__malloc_dump();
#endif

	/* Round size up to an integral number of blocks. */
	size = ((size + MBLOCKSIZE - 1) & ~(size_t)(MBLOCKSIZE-1));

	/*
	 * First-fit search algorithm for available blocks.
	 * Check to make sure the next/previous sizes all agree.
	 */
	rightprevblock = 0;
	mh = NULL;
	for (i=__heapbase; i<__heaptop; i += M_NEXTOFF(mh)) {
		mh = (struct mheader *) i;
		if (!M_OK(mh)) {
			errx(1, "malloc: Heap corrupt; header at 0x%lx"
			     " has bad magic bits",
			     (unsigned long) i);
		}
		if (mh->mh_prevblock != rightprevblock) {
			errx(1, "malloc: Heap corrupt; header at 0x%lx"
			     " has bad previous-block size %lu "
			     "(should be %lu)",
			     (unsigned long) i,
			     (unsigned long) mh->mh_prevblock << MBLOCKSHIFT,
			     (unsigned long) rightprevblock << MBLOCKSHIFT);
		}
		rightprevblock = mh->mh_nextblock;

		/* Can't allocate a block that's in use. */
		if (mh->mh_inuse) {
			continue;
		}

		/* Can't allocate a block that isn't big enough. */
		if (M_SIZE(mh) < size) {
			continue;
		}

		/* Try splitting block. */
		__malloc_split(mh, size);

		/*
		 * Now, allocate.
		 */
		mh->mh_inuse = 1;

#ifdef MALLOCDEBUG
		warnx("malloc: allocating at %p", M_DATA(mh));
		__malloc_dump();
#endif
		return M_DATA(mh);
	}
	if (i!=__heaptop) {
		errx(1, "malloc: Heap corrupt; ran off end");
	}

	/*
	 * Didn't find anything. Expand the heap.
	 *
	 * If the heap is nonempty and the top block (the one mh is
	 * left pointing to after the above loop) is free, we can
	 * expand it. Otherwise we need a new block.
	 */
	if (mh != NULL && !mh->mh_inuse) {
		assert(size > M_SIZE(mh));
		morespace = size - M_SIZE(mh);
	}
	else {
		morespace = MBLOCKSIZE + size;
	}

	/* Round the amount of space we ask for up to a whole page. */
	morespace = PAGE_SIZE * ((morespace + PAGE_SIZE - 1) / PAGE_SIZE);

	p = __malloc_sbrk(morespace);
	if (p == NULL) {
		return NULL;
	}

	if (mh != NULL && !mh->mh_inuse) {
		/* update old header */
		mh->mh_nextblock = M_MKFIELD(M_NEXTOFF(mh) + morespace);
		mh->mh_inuse = 1;
	}
	else {
		/* fill out new header */
		mh = p;
		mh->mh_prevblock = rightprevblock;
		mh->mh_magic1 = MMAGIC;
		mh->mh_magic2 = MMAGIC;
		mh->mh_pad = 0;
		mh->mh_inuse = 1;
		mh->mh_nextblock = M_MKFIELD(morespace);
	}

	/*
	 * Either way, try splitting the block we got as because of
	 * the page rounding it might be quite a bit bigger than we
	 * needed.
	 */
	__malloc_split(mh, size);

#ifdef MALLOCDEBUG
	warnx("malloc: allocating at %p", M_DATA(mh));
	__malloc_dump();
#endif
	return M_DATA(mh);
}

////////////////////////////////////////////////////////////

/*
 * Clear a range of memory with 0xdeadbeef.
 * ptr must be suitably aligned.
 */
static
void
__malloc_deadbeef(void *ptr, size_t size)
{
	uint32_t *x = ptr;
	size_t i, n = size/sizeof(uint32_t);
	for (i=0; i<n; i++) {
		x[i] = 0xdeadbeef;
	}
}

/*
 * Attempt to merge two adjacent blocks (mh below mhnext).
 */
static
void
__malloc_trymerge(struct mheader *mh, struct mheader *mhnext)
{
	struct mheader *mhnextnext;

	if (mh->mh_nextblock != mhnext->mh_prevblock) {
		errx(1, "free: Heap corrupt (%p and %p inconsistent)",
		     mh, mhnext);
	}
	if (mh->mh_inuse || mhnext->mh_inuse) {
		/* can't merge */
		return;
	}

	mhnextnext = M_NEXT(mhnext);

	mh->mh_nextblock = M_MKFIELD(MBLOCKSIZE + M_SIZE(mh) +
				     MBLOCKSIZE + M_SIZE(mhnext));

	if (mhnextnext != (struct mheader *)__heaptop) {
		mhnextnext->mh_prevblock = mh->mh_nextblock;
	}

	/* Deadbeef out the memory used by the now-obsolete header */
	__malloc_deadbeef(mhnext, sizeof(struct mheader));
}

/*
 * The actual free() implementation.
 */
void
free(void *x)
{
	struct mheader *mh, *mhnext, *mhprev;

	if (x==NULL) {
		/* safest practice */
		return;
	}

	/* Consistency check. */
	if (__heapbase==0 || __heaptop==0 || __heapbase > __heaptop) {
		warnx("free: Internal error - local data corrupt");
		errx(1, "free: heapbase 0x%lx; heaptop 0x%lx",
		     (unsigned long) __heapbase, (unsigned long) __heaptop);
	}

	/* Don't allow freeing pointers that aren't on the heap. */
	if ((uintptr_t)x < __heapbase || (uintptr_t)x >= __heaptop) {
		errx(1, "free: Invalid pointer %p freed (out of range)", x);
	}

#ifdef MALLOCDEBUG
	warnx("free: about to free %p", x);
	__malloc_dump();
#endif

	mh = ((struct mheader *)x)-1;
	if (!M_OK(mh)) {
		errx(1, "free: Invalid pointer %p freed (corrupt header)", x);
	}

	if (!mh->mh_inuse) {
		errx(1, "free: Invalid pointer %p freed (already free)", x);
	}

	/* mark it free */
	mh->mh_inuse = 0;

	/* wipe it */
	__malloc_deadbeef(M_DATA(mh), M_SIZE(mh));

	/* Try merging with the block above (but not if we're at the top) */
	mhnext = M_NEXT(mh);
	if (mhnext != (struct mheader *)__heaptop) {
		__malloc_trymerge(mh, mhnext);
	}

	/* Try merging with the block below (but not if we're at the bottom) */
	if (mh != (struct mheader *)__heapbase) {
		mhprev = M_PREV(mh);
		__malloc_trymerge(mhprev, mh);
	}

#ifdef MALLOCDEBUG
	warnx("free: freed %p", x);
	__malloc_dump();
#endif
}
