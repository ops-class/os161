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

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <vm.h>
#include <kern/test161.h>
#include <test.h>

/*
 * Kernel malloc.
 */


/*
 * Fill a block with 0xdeadbeef.
 */
static
void
fill_deadbeef(void *vptr, size_t len)
{
	uint32_t *ptr = vptr;
	size_t i;

	for (i=0; i<len/sizeof(uint32_t); i++) {
		ptr[i] = 0xdeadbeef;
	}
}

////////////////////////////////////////////////////////////
//
// Pool-based subpage allocator.
//
// It works like this:
//
//    We allocate one page at a time and fill it with objects of size k,
//    for various k. Each page has its own freelist, maintained by a
//    linked list in the first word of each object. Each page also has a
//    freecount, so we know when the page is completely free and can
//    release it.
//
//    No assumptions are made about the sizes k; they need not be
//    powers of two. Note, however, that malloc must always return
//    pointers aligned to the maximum alignment requirements of the
//    platform; thus block sizes must at least be multiples of 4,
//    preferably 8. They must also be at least sizeof(struct
//    freelist). It is only worth defining an additional block size if
//    more blocks would fit on a page than with the existing block
//    sizes, and large numbers of items of the new size are allocated.
//
//    The free counts and addresses of the pages are maintained in
//    another list.  Maintaining this table is a nuisance, because it
//    cannot recursively use the subpage allocator. (We could probably
//    make that work, but it would be painful.)
//

////////////////////////////////////////

/*
 * Debugging modes.
 *
 * SLOW enables consistency checks; this will check the integrity of
 * kernel heap pages that kmalloc touches in the course of ordinary
 * operations.
 *
 * SLOWER enables SLOW and also checks the integrity of all heap pages
 * at strategic points.
 *
 * GUARDS enables the use of guard bands on subpage allocations, so as
 * to catch simple off-the-end accesses. By default the guard bands
 * are checked only at kfree() time. This is independent of SLOW and
 * SLOWER. Note that the extra space used by the guard bands increases
 * memory usage (possibly by a lot depending on the sizes allocated) and
 * will likely produce a different heap layout, so it's likely to cause
 * malloc-related bugs to manifest differently.
 *
 * LABELS records the allocation site and a generation number for each
 * allocation and is useful for tracking down memory leaks.
 *
 * On top of these one can enable the following:
 *
 * CHECKBEEF checks that free blocks still contain 0xdeadbeef when
 * checking kernel heap pages with SLOW and SLOWER. This is quite slow
 * in its own right.
 *
 * CHECKGUARDS checks that allocated blocks' guard bands are intact
 * when checking kernel heap pages with SLOW and SLOWER. This is also
 * quite slow in its own right.
 */

#undef  SLOW
#undef SLOWER
#undef GUARDS
#undef LABELS

#undef CHECKBEEF
#undef CHECKGUARDS

////////////////////////////////////////

#if PAGE_SIZE == 4096

#define NSIZES 8
static const size_t sizes[NSIZES] = { 16, 32, 64, 128, 256, 512, 1024, 2048 };

#define SMALLEST_SUBPAGE_SIZE 16
#define LARGEST_SUBPAGE_SIZE 2048

#elif PAGE_SIZE == 8192
#error "No support for 8k pages (yet?)"
#else
#error "Odd page size"
#endif

////////////////////////////////////////

struct freelist {
	struct freelist *next;
};

struct pageref {
	struct pageref *next_samesize;
	struct pageref *next_all;
	vaddr_t pageaddr_and_blocktype;
	uint16_t freelist_offset;
	uint16_t nfree;
};

#define INVALID_OFFSET   (0xffff)

#define PR_PAGEADDR(pr)  ((pr)->pageaddr_and_blocktype & PAGE_FRAME)
#define PR_BLOCKTYPE(pr) ((pr)->pageaddr_and_blocktype & ~PAGE_FRAME)
#define MKPAB(pa, blk)   (((pa)&PAGE_FRAME) | ((blk) & ~PAGE_FRAME))

////////////////////////////////////////

/*
 * Use one spinlock for the whole thing. Making parts of the kmalloc
 * logic per-cpu is worthwhile for scalability; however, for the time
 * being at least we won't, because it adds a lot of complexity and in
 * OS/161 performance and scalability aren't super-critical.
 */

static struct spinlock kmalloc_spinlock = SPINLOCK_INITIALIZER;

////////////////////////////////////////

/*
 * We can only allocate whole pages of pageref structure at a time.
 * This is a struct type for such a page.
 *
 * Each pageref page contains 256 pagerefs, which can manage up to
 * 256 * 4K = 1M of kernel heap.
 */

#define NPAGEREFS_PER_PAGE (PAGE_SIZE / sizeof(struct pageref))

struct pagerefpage {
	struct pageref refs[NPAGEREFS_PER_PAGE];
};

/*
 * This structure holds a pointer to a pageref page and also its
 * bitmap of free entries.
 */

#define INUSE_WORDS (NPAGEREFS_PER_PAGE / 32)

struct kheap_root {
	struct pagerefpage *page;
	uint32_t pagerefs_inuse[INUSE_WORDS];
	unsigned numinuse;
};

/*
 * It would be better to make this dynamically sizeable. However,
 * since we only actually run on System/161 and System/161 is
 * specifically limited to 16M of RAM, we'll just adopt that as a
 * static size limit.
 *
 * FUTURE: it would be better to pick this number based on the RAM
 * size we find at boot time.
 */

#define NUM_PAGEREFPAGES 16
#define TOTAL_PAGEREFS (NUM_PAGEREFPAGES * NPAGEREFS_PER_PAGE)

static struct kheap_root kheaproots[NUM_PAGEREFPAGES];

/*
 * Allocate a page to hold pagerefs.
 */
static
void
allocpagerefpage(struct kheap_root *root)
{
	vaddr_t va;

	KASSERT(root->page == NULL);

	/*
	 * We release the spinlock while calling alloc_kpages. This
	 * avoids deadlock if alloc_kpages needs to come back here.
	 * Note that this means things can change behind our back...
	 */
	spinlock_release(&kmalloc_spinlock);
	va = alloc_kpages(1);
	spinlock_acquire(&kmalloc_spinlock);
	if (va == 0) {
		kprintf("kmalloc: Couldn't get a pageref page\n");
		return;
	}
	KASSERT(va % PAGE_SIZE == 0);

	if (root->page != NULL) {
		/* Oops, somebody else allocated it. */
		spinlock_release(&kmalloc_spinlock);
		free_kpages(va);
		spinlock_acquire(&kmalloc_spinlock);
		/* Once allocated it isn't ever freed. */
		KASSERT(root->page != NULL);
		return;
	}

	root->page = (struct pagerefpage *)va;
}

/*
 * Allocate a pageref structure.
 */
static
struct pageref *
allocpageref(void)
{
	unsigned i,j;
	uint32_t k;
	unsigned whichroot;
	struct kheap_root *root;

	for (whichroot=0; whichroot < NUM_PAGEREFPAGES; whichroot++) {
		root = &kheaproots[whichroot];
		if (root->numinuse >= NPAGEREFS_PER_PAGE) {
			continue;
		}

		/*
		 * This should probably not be a linear search.
		 */
		for (i=0; i<INUSE_WORDS; i++) {
			if (root->pagerefs_inuse[i]==0xffffffff) {
				/* full */
				continue;
			}
			for (k=1,j=0; k!=0; k<<=1,j++) {
				if ((root->pagerefs_inuse[i] & k)==0) {
					root->pagerefs_inuse[i] |= k;
					root->numinuse++;
					if (root->page == NULL) {
						allocpagerefpage(root);
					}
					if (root->page == NULL) {
						return NULL;
					}
					return &root->page->refs[i*32 + j];
				}
			}
			KASSERT(0);
		}
	}

	/* ran out */
	return NULL;
}

/*
 * Release a pageref structure.
 */
static
void
freepageref(struct pageref *p)
{
	size_t i, j;
	uint32_t k;
	unsigned whichroot;
	struct kheap_root *root;
	struct pagerefpage *page;

	for (whichroot=0; whichroot < NUM_PAGEREFPAGES; whichroot++) {
		root = &kheaproots[whichroot];

		page = root->page;
		if (page == NULL) {
			KASSERT(root->numinuse == 0);
			continue;
		}

		j = p-page->refs;
		/* note: j is unsigned, don't test < 0 */
		if (j < NPAGEREFS_PER_PAGE) {
			/* on this page */
			i = j/32;
			k = ((uint32_t)1) << (j%32);
			KASSERT((root->pagerefs_inuse[i] & k) != 0);
			root->pagerefs_inuse[i] &= ~k;
			KASSERT(root->numinuse > 0);
			root->numinuse--;
			return;
		}
	}
	/* pageref wasn't on any of the pages */
	KASSERT(0);
}

////////////////////////////////////////

/*
 * Each pageref is on two linked lists: one list of pages of blocks of
 * that same size, and one of all blocks.
 */
static struct pageref *sizebases[NSIZES];
static struct pageref *allbase;

////////////////////////////////////////

#ifdef GUARDS

/* Space returned to the client is filled with GUARD_RETBYTE */
#define GUARD_RETBYTE 0xa9
/* Padding space (internal fragmentation loss) is filled with GUARD_FILLBYTE */
#define GUARD_FILLBYTE 0xba
/* The guard bands on an allocated block should contain GUARD_HALFWORD */
#define GUARD_HALFWORD 0xb0b0

/* The guard scheme uses 8 bytes per block. */
#define GUARD_OVERHEAD 8

/* Pointers are offset by 4 bytes when guards are in use. */
#define GUARD_PTROFFSET 4

/*
 * Set up the guard values in a block we're about to return.
 */
static
void *
establishguardband(void *block, size_t clientsize, size_t blocksize)
{
	vaddr_t lowguard, lowsize, data, enddata, highguard, highsize, i;

	KASSERT(clientsize + GUARD_OVERHEAD <= blocksize);
	KASSERT(clientsize < 65536U);

	lowguard = (vaddr_t)block;
	lowsize = lowguard + 2;
	data = lowsize + 2;
	enddata = data + clientsize;
	highguard = lowguard + blocksize - 4;
	highsize = highguard + 2;

	*(uint16_t *)lowguard = GUARD_HALFWORD;
	*(uint16_t *)lowsize = clientsize;
	for (i=data; i<enddata; i++) {
		*(uint8_t *)i = GUARD_RETBYTE;
	}
	for (i=enddata; i<highguard; i++) {
		*(uint8_t *)i = GUARD_FILLBYTE;
	}
	*(uint16_t *)highguard = GUARD_HALFWORD;
	*(uint16_t *)highsize = clientsize;

	return (void *)data;
}

/*
 * Validate the guard values in an existing block.
 */
static
void
checkguardband(vaddr_t blockaddr, size_t smallerblocksize, size_t blocksize)
{
	/*
	 * The first two bytes of the block are the lower guard band.
	 * The next two bytes are the real size (the size of the
	 * client data). The last four bytes of the block duplicate
	 * this info. In between the real data and the upper guard
	 * band should be filled with GUARD_FILLBYTE.
	 *
	 * If the guard values are wrong, or the low and high sizes
	 * don't match, or the size is out of range, by far the most
	 * likely explanation is that something ran past the bounds of
	 * its memory block.
	 */
	vaddr_t lowguard, lowsize, data, enddata, highguard, highsize, i;
	unsigned clientsize;

	lowguard = blockaddr;
	lowsize = lowguard + 2;
	data = lowsize + 2;
	highguard = blockaddr + blocksize - 4;
	highsize = highguard + 2;

	KASSERT(*(uint16_t *)lowguard == GUARD_HALFWORD);
	KASSERT(*(uint16_t *)highguard == GUARD_HALFWORD);
	clientsize = *(uint16_t *)lowsize;
	KASSERT(clientsize == *(uint16_t *)highsize);
	KASSERT(clientsize + GUARD_OVERHEAD > smallerblocksize);
	KASSERT(clientsize + GUARD_OVERHEAD <= blocksize);
	enddata = data + clientsize;
	for (i=enddata; i<highguard; i++) {
		KASSERT(*(uint8_t *)i == GUARD_FILLBYTE);
	}
}

#else /* not GUARDS */

#define GUARD_OVERHEAD 0

#endif /* GUARDS */

////////////////////////////////////////

/* SLOWER implies SLOW */
#ifdef SLOWER
#ifndef SLOW
#define SLOW
#endif
#endif

#ifdef CHECKBEEF
/*
 * Check that a (free) block contains deadbeef as it should.
 *
 * The first word of the block is a freelist pointer and should not be
 * deadbeef; the rest of the block should be only deadbeef.
 */
static
void
checkdeadbeef(void *block, size_t blocksize)
{
	uint32_t *ptr = block;
	size_t i;

	for (i=1; i < blocksize/sizeof(uint32_t); i++) {
		KASSERT(ptr[i] == 0xdeadbeef);
	}
}
#endif /* CHECKBEEF */

#ifdef SLOW
/*
 * Check that a particular heap page (the one managed by the argument
 * PR) is valid.
 *
 * This checks:
 *    - that the page is within MIPS_KSEG0 (for mips)
 *    - that the freelist starting point in PR is valid
 *    - that the number of free blocks is consistent with the freelist
 *    - that each freelist next pointer points within the page
 *    - that no freelist pointer points to the middle of a block
 *    - that free blocks are still deadbeefed (if CHECKBEEF)
 *    - that the freelist is not circular
 *    - that the guard bands are intact on all allocated blocks (if
 *      CHECKGUARDS)
 *
 * Note that if CHECKGUARDS is set, a circular freelist will cause an
 * assertion as a bit in isfree is set twice; if not, a circular
 * freelist will cause an infinite loop.
 */
static
void
checksubpage(struct pageref *pr)
{
	vaddr_t prpage, fla;
	struct freelist *fl;
	int blktype;
	int nfree=0;
	size_t blocksize;
#ifdef CHECKGUARDS
	const unsigned maxblocks = PAGE_SIZE / SMALLEST_SUBPAGE_SIZE;
	const unsigned numfreewords = DIVROUNDUP(maxblocks, 32);
	uint32_t isfree[numfreewords], mask;
	unsigned numblocks, blocknum, i;
	size_t smallerblocksize;
#endif

	KASSERT(spinlock_do_i_hold(&kmalloc_spinlock));

	if (pr->freelist_offset == INVALID_OFFSET) {
		KASSERT(pr->nfree==0);
		return;
	}

	prpage = PR_PAGEADDR(pr);
	blktype = PR_BLOCKTYPE(pr);
	KASSERT(blktype >= 0 && blktype < NSIZES);
	blocksize = sizes[blktype];

#ifdef CHECKGUARDS
	smallerblocksize = blktype > 0 ? sizes[blktype - 1] : 0;
	for (i=0; i<numfreewords; i++) {
		isfree[i] = 0;
	}
#endif

#ifdef __mips__
	KASSERT(prpage >= MIPS_KSEG0);
	KASSERT(prpage < MIPS_KSEG1);
#endif

	KASSERT(pr->freelist_offset < PAGE_SIZE);
	KASSERT(pr->freelist_offset % blocksize == 0);

	fla = prpage + pr->freelist_offset;
	fl = (struct freelist *)fla;

	for (; fl != NULL; fl = fl->next) {
		fla = (vaddr_t)fl;
		KASSERT(fla >= prpage && fla < prpage + PAGE_SIZE);
		KASSERT((fla-prpage) % blocksize == 0);
#ifdef CHECKBEEF
		checkdeadbeef(fl, blocksize);
#endif
#ifdef CHECKGUARDS
		blocknum = (fla-prpage) / blocksize;
		mask = 1U << (blocknum % 32);
		KASSERT((isfree[blocknum / 32] & mask) == 0);
		isfree[blocknum / 32] |= mask;
#endif
		KASSERT(fl->next != fl);
		nfree++;
	}
	KASSERT(nfree==pr->nfree);

#ifdef CHECKGUARDS
	numblocks = PAGE_SIZE / blocksize;
	for (i=0; i<numblocks; i++) {
		mask = 1U << (i % 32);
		if ((isfree[i / 32] & mask) == 0) {
			checkguardband(prpage + i * blocksize,
				       smallerblocksize, blocksize);
		}
	}
#endif
}
#else
#define checksubpage(pr) ((void)(pr))
#endif

#ifdef SLOWER
/*
 * Run checksubpage on all heap pages. This also checks that the
 * linked lists of pagerefs are more or less intact.
 */
static
void
checksubpages(void)
{
	struct pageref *pr;
	int i;
	unsigned sc=0, ac=0;

	KASSERT(spinlock_do_i_hold(&kmalloc_spinlock));

	for (i=0; i<NSIZES; i++) {
		for (pr = sizebases[i]; pr != NULL; pr = pr->next_samesize) {
			checksubpage(pr);
			KASSERT(sc < TOTAL_PAGEREFS);
			sc++;
		}
	}

	for (pr = allbase; pr != NULL; pr = pr->next_all) {
		checksubpage(pr);
		KASSERT(ac < TOTAL_PAGEREFS);
		ac++;
	}

	KASSERT(sc==ac);
}
#else
#define checksubpages()
#endif

////////////////////////////////////////

#ifdef LABELS

#define LABEL_PTROFFSET sizeof(struct malloclabel)
#define LABEL_OVERHEAD LABEL_PTROFFSET

struct malloclabel {
	vaddr_t label;
	unsigned generation;
};

static unsigned mallocgeneration;

/*
 * Label a block of memory.
 */
static
void *
establishlabel(void *block, vaddr_t label)
{
	struct malloclabel *ml;

	ml = block;
	ml->label = label;
	ml->generation = mallocgeneration;
	ml++;
	return ml;
}

static
void
dump_subpage(struct pageref *pr, unsigned generation)
{
	unsigned blocksize = sizes[PR_BLOCKTYPE(pr)];
	unsigned numblocks = PAGE_SIZE / blocksize;
	unsigned numfreewords = DIVROUNDUP(numblocks, 32);
	uint32_t isfree[numfreewords], mask;
	vaddr_t prpage;
	struct freelist *fl;
	vaddr_t blockaddr;
	struct malloclabel *ml;
	unsigned i;

	for (i=0; i<numfreewords; i++) {
		isfree[i] = 0;
	}

	prpage = PR_PAGEADDR(pr);
	fl = (struct freelist *)(prpage + pr->freelist_offset);
	for (; fl != NULL; fl = fl->next) {
		i = ((vaddr_t)fl - prpage) / blocksize;
		mask = 1U << (i % 32);
		isfree[i / 32] |= mask;
	}

	for (i=0; i<numblocks; i++) {
		mask = 1U << (i % 32);
		if (isfree[i / 32] & mask) {
			continue;
		}
		blockaddr = prpage + i * blocksize;
		ml = (struct malloclabel *)blockaddr;
		if (ml->generation != generation) {
			continue;
		}
		kprintf("%5zu bytes at %p, allocated at %p\n",
			blocksize, (void *)blockaddr, (void *)ml->label);
	}
}

static
void
dump_subpages(unsigned generation)
{
	struct pageref *pr;
	int i;

	kprintf("Remaining allocations from generation %u:\n", generation);
	for (i=0; i<NSIZES; i++) {
		for (pr = sizebases[i]; pr != NULL; pr = pr->next_samesize) {
			dump_subpage(pr, generation);
		}
	}
}

#else

#define LABEL_OVERHEAD 0

#endif /* LABELS */

void
kheap_nextgeneration(void)
{
#ifdef LABELS
	spinlock_acquire(&kmalloc_spinlock);
	mallocgeneration++;
	spinlock_release(&kmalloc_spinlock);
#endif
}

void
kheap_dump(void)
{
#ifdef LABELS
	/* print the whole thing with interrupts off */
	spinlock_acquire(&kmalloc_spinlock);
	dump_subpages(mallocgeneration);
	spinlock_release(&kmalloc_spinlock);
#else
	kprintf("Enable LABELS in kmalloc.c to use this functionality.\n");
#endif
}

void
kheap_dumpall(void)
{
#ifdef LABELS
	unsigned i;

	/* print the whole thing with interrupts off */
	spinlock_acquire(&kmalloc_spinlock);
	for (i=0; i<=mallocgeneration; i++) {
		dump_subpages(i);
	}
	spinlock_release(&kmalloc_spinlock);
#else
	kprintf("Enable LABELS in kmalloc.c to use this functionality.\n");
#endif
}

////////////////////////////////////////

/*
 * Print the allocated/freed map of a single kernel heap page.
 */
static
unsigned long
subpage_stats(struct pageref *pr, bool quiet)
{
	vaddr_t prpage, fla;
	struct freelist *fl;
	int blktype;
	unsigned i, n, index;
	uint32_t freemap[PAGE_SIZE / (SMALLEST_SUBPAGE_SIZE*32)];

	checksubpage(pr);
	KASSERT(spinlock_do_i_hold(&kmalloc_spinlock));

	/* clear freemap[] */
	for (i=0; i<ARRAYCOUNT(freemap); i++) {
		freemap[i] = 0;
	}

	prpage = PR_PAGEADDR(pr);
	blktype = PR_BLOCKTYPE(pr);
	KASSERT(blktype >= 0 && blktype < NSIZES);

	/* compute how many bits we need in freemap and assert we fit */
	n = PAGE_SIZE / sizes[blktype];
	KASSERT(n <= 32 * ARRAYCOUNT(freemap));

	if (pr->freelist_offset != INVALID_OFFSET) {
		fla = prpage + pr->freelist_offset;
		fl = (struct freelist *)fla;

		for (; fl != NULL; fl = fl->next) {
			fla = (vaddr_t)fl;
			index = (fla-prpage) / sizes[blktype];
			KASSERT(index<n);
			freemap[index/32] |= (1<<(index%32));
		}
	}

	if (!quiet) {
		kprintf("at 0x%08lx: size %-4lu  %u/%u free\n",
				(unsigned long)prpage, (unsigned long) sizes[blktype],
				(unsigned) pr->nfree, n);
		kprintf("   ");
		for (i=0; i<n; i++) {
			int val = (freemap[i/32] & (1<<(i%32)))!=0;
			kprintf("%c", val ? '.' : '*');
			if (i%64==63 && i<n-1) {
				kprintf("\n   ");
			}
		}
		kprintf("\n");
	}
	return ((unsigned long)sizes[blktype] * (n - (unsigned) pr->nfree));
}

/*
 * Print the whole heap.
 */
void
kheap_printstats(void)
{
	struct pageref *pr;

	/* print the whole thing with interrupts off */
	spinlock_acquire(&kmalloc_spinlock);

	kprintf("Subpage allocator status:\n");

	for (pr = allbase; pr != NULL; pr = pr->next_all) {
		subpage_stats(pr, false);
	}

	spinlock_release(&kmalloc_spinlock);
}


/*
 * Return the number of used bytes.
 */

unsigned long
kheap_getused(void) {
	struct pageref *pr;
	unsigned long total = 0;
	unsigned int num_pages = 0, coremap_bytes = 0;

	/* compute with interrupts off */
	spinlock_acquire(&kmalloc_spinlock);
	for (pr = allbase; pr != NULL; pr = pr->next_all) {
		total += subpage_stats(pr, true);
		num_pages++;
	}

	coremap_bytes = coremap_used_bytes();

	// Don't double-count the pages we're using for subpage allocation;
	// we've already accounted for the used portion.
	if (coremap_bytes > 0) {
		total += coremap_bytes - (num_pages * PAGE_SIZE);
	}

	spinlock_release(&kmalloc_spinlock);

	return total;
}

/*
 * Print number of used bytes.
 */

void
kheap_printused(void)
{
	char total_string[32];
	snprintf(total_string, sizeof(total_string), "%lu", kheap_getused());
	secprintf(SECRET, total_string, "khu");
}

////////////////////////////////////////

/*
 * Remove a pageref from both lists that it's on.
 */
static
void
remove_lists(struct pageref *pr, int blktype)
{
	struct pageref **guy;

	KASSERT(blktype>=0 && blktype<NSIZES);

	for (guy = &sizebases[blktype]; *guy; guy = &(*guy)->next_samesize) {
		checksubpage(*guy);
		if (*guy == pr) {
			*guy = pr->next_samesize;
			break;
		}
	}

	for (guy = &allbase; *guy; guy = &(*guy)->next_all) {
		checksubpage(*guy);
		if (*guy == pr) {
			*guy = pr->next_all;
			break;
		}
	}
}

/*
 * Given a requested client size, return the block type, that is, the
 * index into the sizes[] array for the block size to use.
 */
static
inline
int blocktype(size_t clientsz)
{
	unsigned i;
	for (i=0; i<NSIZES; i++) {
		if (clientsz <= sizes[i]) {
			return i;
		}
	}

	panic("Subpage allocator cannot handle allocation of size %zu\n",
	      clientsz);

	// keep compiler happy
	return 0;
}

/*
 * Allocate a block of size SZ, where SZ is not large enough to
 * warrant a whole-page allocation.
 */
static
void *
subpage_kmalloc(size_t sz
#ifdef LABELS
		, vaddr_t label
#endif
	)
{
	unsigned blktype;	// index into sizes[] that we're using
	struct pageref *pr;	// pageref for page we're allocating from
	vaddr_t prpage;		// PR_PAGEADDR(pr)
	vaddr_t fla;		// free list entry address
	struct freelist *volatile fl;	// free list entry
	void *retptr;		// our result

	volatile int i;

#ifdef GUARDS
	size_t clientsz;
#endif

#ifdef GUARDS
	clientsz = sz;
	sz += GUARD_OVERHEAD;
#endif
#ifdef LABELS
#ifdef GUARDS
	/* Include the label in what GUARDS considers the client data. */
	clientsz += LABEL_PTROFFSET;
#endif
	sz += LABEL_PTROFFSET;
#endif
	blktype = blocktype(sz);
#ifdef GUARDS
	sz = sizes[blktype];
#endif

	spinlock_acquire(&kmalloc_spinlock);

	checksubpages();

	for (pr = sizebases[blktype]; pr != NULL; pr = pr->next_samesize) {

		/* check for corruption */
		KASSERT(PR_BLOCKTYPE(pr) == blktype);
		checksubpage(pr);

		if (pr->nfree > 0) {

		doalloc: /* comes here after getting a whole fresh page */

			KASSERT(pr->freelist_offset < PAGE_SIZE);
			prpage = PR_PAGEADDR(pr);
			fla = prpage + pr->freelist_offset;
			fl = (struct freelist *)fla;

			retptr = fl;
			fl = fl->next;
			pr->nfree--;

			if (fl != NULL) {
				KASSERT(pr->nfree > 0);
				fla = (vaddr_t)fl;
				KASSERT(fla - prpage < PAGE_SIZE);
				pr->freelist_offset = fla - prpage;
			}
			else {
				KASSERT(pr->nfree == 0);
				pr->freelist_offset = INVALID_OFFSET;
			}
#ifdef GUARDS
			retptr = establishguardband(retptr, clientsz, sz);
#endif
#ifdef LABELS
			retptr = establishlabel(retptr, label);
#endif

			checksubpages();

			spinlock_release(&kmalloc_spinlock);
			return retptr;
		}
	}

	/*
	 * No page of the right size available.
	 * Make a new one.
	 *
	 * We release the spinlock while calling alloc_kpages. This
	 * avoids deadlock if alloc_kpages needs to come back here.
	 * Note that this means things can change behind our back...
	 */

	spinlock_release(&kmalloc_spinlock);
	prpage = alloc_kpages(1);
	if (prpage==0) {
		/* Out of memory. */
		silent("kmalloc: Subpage allocator couldn't get a page\n");
		return NULL;
	}
	KASSERT(prpage % PAGE_SIZE == 0);
#ifdef CHECKBEEF
	/* deadbeef the whole page, as it probably starts zeroed */
	fill_deadbeef((void *)prpage, PAGE_SIZE);
#endif
	spinlock_acquire(&kmalloc_spinlock);

	pr = allocpageref();
	if (pr==NULL) {
		/* Couldn't allocate accounting space for the new page. */
		spinlock_release(&kmalloc_spinlock);
		free_kpages(prpage);
		kprintf("kmalloc: Subpage allocator couldn't get pageref\n");
		return NULL;
	}

	pr->pageaddr_and_blocktype = MKPAB(prpage, blktype);
	pr->nfree = PAGE_SIZE / sizes[blktype];

	/*
	 * Note: fl is volatile because the MIPS toolchain we were
	 * using in spring 2001 attempted to optimize this loop and
	 * blew it. Making fl volatile inhibits the optimization.
	 */

	fla = prpage;
	fl = (struct freelist *)fla;
	fl->next = NULL;
	for (i=1; i<pr->nfree; i++) {
		fl = (struct freelist *)(fla + i*sizes[blktype]);
		fl->next = (struct freelist *)(fla + (i-1)*sizes[blktype]);
		KASSERT(fl != fl->next);
	}
	fla = (vaddr_t) fl;
	pr->freelist_offset = fla - prpage;
	KASSERT(pr->freelist_offset == (pr->nfree-1)*sizes[blktype]);

	pr->next_samesize = sizebases[blktype];
	sizebases[blktype] = pr;

	pr->next_all = allbase;
	allbase = pr;

	/* This is kind of cheesy, but avoids duplicating the alloc code. */
	goto doalloc;
}

/*
 * Free a pointer previously returned from subpage_kmalloc. If the
 * pointer is not on any heap page we recognize, return -1.
 */
static
int
subpage_kfree(void *ptr)
{
	int blktype;		// index into sizes[] that we're using
	vaddr_t ptraddr;	// same as ptr
	struct pageref *pr;	// pageref for page we're freeing in
	vaddr_t prpage;		// PR_PAGEADDR(pr)
	vaddr_t fla;		// free list entry address
	struct freelist *fl;	// free list entry
	vaddr_t offset;		// offset into page
#ifdef GUARDS
	size_t blocksize, smallerblocksize;
#endif

	ptraddr = (vaddr_t)ptr;
#ifdef GUARDS
	if (ptraddr % PAGE_SIZE == 0) {
		/*
		 * With guard bands, all client-facing subpage
		 * pointers are offset by GUARD_PTROFFSET (which is 4)
		 * from the underlying blocks and are therefore not
		 * page-aligned. So a page-aligned pointer is not one
		 * of ours. Catch this up front, as otherwise
		 * subtracting GUARD_PTROFFSET could give a pointer on
		 * a page we *do* own, and then we'll panic because
		 * it's not a valid one.
		 */
		return -1;
	}
	ptraddr -= GUARD_PTROFFSET;
#endif
#ifdef LABELS
	if (ptraddr % PAGE_SIZE == 0) {
		/* ditto */
		return -1;
	}
	ptraddr -= LABEL_PTROFFSET;
#endif

	spinlock_acquire(&kmalloc_spinlock);

	checksubpages();

	/* Silence warnings with gcc 4.8 -Og (but not -O2) */
	prpage = 0;
	blktype = 0;

	for (pr = allbase; pr; pr = pr->next_all) {
		prpage = PR_PAGEADDR(pr);
		blktype = PR_BLOCKTYPE(pr);
		KASSERT(blktype >= 0 && blktype < NSIZES);

		/* check for corruption */
		KASSERT(blktype>=0 && blktype<NSIZES);
		checksubpage(pr);

		if (ptraddr >= prpage && ptraddr < prpage + PAGE_SIZE) {
			break;
		}
	}

	if (pr==NULL) {
		/* Not on any of our pages - not a subpage allocation */
		spinlock_release(&kmalloc_spinlock);
		return -1;
	}

	offset = ptraddr - prpage;

	/* Check for proper positioning and alignment */
	if (offset >= PAGE_SIZE || offset % sizes[blktype] != 0) {
		panic("kfree: subpage free of invalid addr %p\n", ptr);
	}

#ifdef GUARDS
	blocksize = sizes[blktype];
	smallerblocksize = blktype > 0 ? sizes[blktype - 1] : 0;
	checkguardband(ptraddr, smallerblocksize, blocksize);
#endif

	/*
	 * Clear the block to 0xdeadbeef to make it easier to detect
	 * uses of dangling pointers.
	 */
	fill_deadbeef((void *)ptraddr, sizes[blktype]);

	/*
	 * We probably ought to check for free twice by seeing if the block
	 * is already on the free list. But that's expensive, so we don't.
	 */

	fla = prpage + offset;
	fl = (struct freelist *)fla;
	if (pr->freelist_offset == INVALID_OFFSET) {
		fl->next = NULL;
	} else {
		fl->next = (struct freelist *)(prpage + pr->freelist_offset);

		/* this block should not already be on the free list! */
#ifdef SLOW
		{
			struct freelist *fl2;

			for (fl2 = fl->next; fl2 != NULL; fl2 = fl2->next) {
				KASSERT(fl2 != fl);
			}
		}
#else
		/* check just the head */
		KASSERT(fl != fl->next);
#endif
	}
	pr->freelist_offset = offset;
	pr->nfree++;

	KASSERT(pr->nfree <= PAGE_SIZE / sizes[blktype]);
	if (pr->nfree == PAGE_SIZE / sizes[blktype]) {
		/* Whole page is free. */
		remove_lists(pr, blktype);
		freepageref(pr);
		/* Call free_kpages without kmalloc_spinlock. */
		spinlock_release(&kmalloc_spinlock);
		free_kpages(prpage);
	}
	else {
		spinlock_release(&kmalloc_spinlock);
	}

#ifdef SLOWER /* Don't get the lock unless checksubpages does something. */
	spinlock_acquire(&kmalloc_spinlock);
	checksubpages();
	spinlock_release(&kmalloc_spinlock);
#endif

	return 0;
}

//
////////////////////////////////////////////////////////////

/*
 * Allocate a block of size SZ. Redirect either to subpage_kmalloc or
 * alloc_kpages depending on how big SZ is.
 */
void *
kmalloc(size_t sz)
{
	size_t checksz;
#ifdef LABELS
	vaddr_t label;
#endif

#ifdef LABELS
#ifdef __GNUC__
	label = (vaddr_t)__builtin_return_address(0);
#else
#error "Don't know how to get return address with this compiler"
#endif /* __GNUC__ */
#endif /* LABELS */

	checksz = sz + GUARD_OVERHEAD + LABEL_OVERHEAD;
	if (checksz >= LARGEST_SUBPAGE_SIZE) {
		unsigned long npages;
		vaddr_t address;

		/* Round up to a whole number of pages. */
		npages = (sz + PAGE_SIZE - 1)/PAGE_SIZE;
		address = alloc_kpages(npages);
		if (address==0) {
			return NULL;
		}
		KASSERT(address % PAGE_SIZE == 0);

		return (void *)address;
	}

#ifdef LABELS
	return subpage_kmalloc(sz, label);
#else
	return subpage_kmalloc(sz);
#endif
}

/*
 * Free a block previously returned from kmalloc.
 */
void
kfree(void *ptr)
{
	/*
	 * Try subpage first; if that fails, assume it's a big allocation.
	 */
	if (ptr == NULL) {
		return;
	} else if (subpage_kfree(ptr)) {
		KASSERT((vaddr_t)ptr%PAGE_SIZE==0);
		free_kpages((vaddr_t)ptr);
	}
}

