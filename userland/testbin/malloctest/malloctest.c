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
 * malloctest.c
 *
 * This program contains a variety of tests for malloc and free.
 * XXX: most tests leak on error.
 *
 * These tests (subject to restrictions and limitations noted below)
 * should work once the kernel provides sbrk().
 *
 * Note that because the userlevel malloc is extremely dumb,
 * malloctest 3 is extremely slow and on most VM systems will run more
 * or less forever.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <err.h>


#define _PATH_RANDOM   "random:"

#define SMALLSIZE   72
#define MEDIUMSIZE  896
#define BIGSIZE     16384
#define HUGESIZE    (1024 * 1024 * 1024)

/* Maximum amount of space per block we allow for indexing structures */
#define OVERHEAD         32

/* Point past which we assume something else is going on */
#define ABSURD_OVERHEAD  256

static
int
geti(void)
{
	int val=0;
	int ch, digits=0;

	while (1) {
		ch = getchar();
		if (ch=='\n' || ch=='\r') {
			putchar('\n');
			break;
		}
		else if ((ch=='\b' || ch==127) && digits>0) {
			tprintf("\b \b");
			val = val/10;
			digits--;
		}
		else if (ch>='0' && ch<='9') {
			putchar(ch);
			val = val*10 + (ch-'0');
			digits++;
		}
		else {
			putchar('\a');
		}
	}

	if (digits==0) {
		return -1;
	}
	return val;
}

////////////////////////////////////////////////////////////

/*
 * Fill a block of memory with a test pattern.
 */
static
void
markblock(volatile void *ptr, size_t size, unsigned bias, int doprint)
{
	size_t n, i;
	unsigned long *pl;
	unsigned long val;

	pl = (unsigned long *)ptr;
	n = size / sizeof(unsigned long);

	for (i=0; i<n; i++) {
		val = ((unsigned long)i ^ (unsigned long)bias);
		pl[i] = val;
		if (doprint && (i%64==63)) {
			tprintf(".");
		}
	}
	if (doprint) {
		tprintf("\n");
	}
}

/*
 * Check a block marked with markblock()
 */
static
int
checkblock(volatile void *ptr, size_t size, unsigned bias, int doprint)
{
	size_t n, i;
	unsigned long *pl;
	unsigned long val;

	pl = (unsigned long *)ptr;
	n = size / sizeof(unsigned long);

	for (i=0; i<n; i++) {
		val = ((unsigned long)i ^ (unsigned long)bias);
		if (pl[i] != val) {
			if (doprint) {
				tprintf("\n");
			}
			tprintf("FAILED: data mismatch at offset %lu of block "
			       "at 0x%lx: %lu vs. %lu\n",
			       (unsigned long) (i*sizeof(unsigned long)),
			       (unsigned long)(uintptr_t)pl,
			       pl[i], val);
			return -1;
		}
		if (doprint && (i%64==63)) {
			tprintf(".");
		}
	}
	if (doprint) {
		tprintf("\n");
	}

	return 0;
}

////////////////////////////////////////////////////////////

/*
 * Test 1
 *
 * This test checks if all the bytes we asked for are getting
 * allocated.
 */

static
void
test1(void)
{
	volatile unsigned *x;

	tprintf("*** Malloc test 1 ***\n");
	tprintf("Allocating %u bytes\n", BIGSIZE);
	x = malloc(BIGSIZE);
	if (x==NULL) {
		tprintf("FAILED: malloc failed\n");
		return;
	}

	markblock(x, BIGSIZE, 0, 0);
	if (checkblock(x, BIGSIZE, 0, 0)) {
		tprintf("FAILED: data corrupt\n");
		return;
	}

	free((void *)x);

	tprintf("Passed malloc test 1.\n");
}


////////////////////////////////////////////////////////////

/*
 * Test 2
 *
 * Tests if malloc gracefully handles failing requests.
 *
 * This test assumes that one of the following conditions holds:
 *    1. swap is not overcommitted; or
 *    2. user processes are limited to some maximum size, and enough
 *       swap exists to hold a maximal user process.
 *
 * That is, it assumes that malloc returns NULL when out of memory,
 * and that the process will not be killed for running out of
 * memory/swap at other times.
 *
 * If mallocing more memory than the system can actually provide
 * backing for succeeds, this test will blow up. That's ok, but please
 * provide a way to switch on one of the above conditions so this test
 * can be run.
 *
 * This test works by trying a huge malloc, and then trying
 * successively smaller mallocs until one works. Then it touches the
 * whole block to make sure the memory is actually successfully
 * allocated. Then it frees the block and allocates it again, which
 * should succeed.
 *
 * Note that this test may give spurious failures if anything else is
 * running at the same time and changing the amount of memory
 * available.
 */

static
void
test2(void)
{
	volatile unsigned *x;
	size_t size;

	tprintf("Entering malloc test 2.\n");
	tprintf("Make sure you read and understand the comment in malloctest.c "
	       "that\nexplains the conditions this test assumes.\n\n");

	tprintf("Testing how much memory we can allocate:\n");

	for (size = HUGESIZE; (x = malloc(size))==NULL; size = size/2) {
		tprintf("  %9lu bytes: failed\n", (unsigned long) size);
	}
	tprintf("  %9lu bytes: succeeded\n", (unsigned long) size);

	tprintf("Passed part 1\n");

	tprintf("Touching all the words in the block.\n");
	markblock(x, size, 0, 1);

	tprintf("Validating the words in the block.\n");
	if (checkblock(x, size, 0, 1)) {
		tprintf("FAILED: data corrupt\n");
		return;
	}
	tprintf("Passed part 2\n");


	tprintf("Freeing the block\n");
	free((void *)x);
	tprintf("Passed part 3\n");
	tprintf("Allocating another block\n");

	x = malloc(size);
	if (x==NULL) {
		tprintf("FAILED: free didn't return the memory?\n");
		return;
	}
	free((void *)x);

	tprintf("Passed malloc test 2.\n");
}


////////////////////////////////////////////////////////////

/*
 * Test 3
 *
 * Tests if malloc gracefully handles failing requests.
 *
 * This test assumes the same conditions as test 2.
 *
 * This test works by mallocing a lot of small blocks in a row until
 * malloc starts failing.
 */

struct test3 {
	struct test3 *next;
	char junk[(SMALLSIZE - sizeof(struct test3 *))];
};

static
void
test3(void)
{
	struct test3 *list = NULL, *tmp;
	size_t tot=0;
	int ct=0, failed=0;
	void *x;

	tprintf("Entering malloc test 3.\n");
	tprintf("Make sure you read and understand the comment in malloctest.c "
	       "that\nexplains the conditions this test assumes.\n\n");

	tprintf("Testing how much memory we can allocate:\n");

	while ((tmp = malloc(sizeof(struct test3))) != NULL) {

		assert(tmp != list);
		tmp->next = list;
		list = tmp;

		tot += sizeof(struct test3);

		markblock(list->junk, sizeof(list->junk), (uintptr_t)list, 0);

		ct++;
		if (ct%128==0) {
			tprintf(".");
		}
	}

	tprintf("Allocated %lu bytes\n", (unsigned long) tot);

	tprintf("Trying some more allocations which I expect to fail...\n");

	x = malloc(SMALLSIZE);
	if (x != NULL) {
		tprintf("FAILED: malloc(%u) succeeded\n", SMALLSIZE);
		return;
	}

	x = malloc(MEDIUMSIZE);
	if (x != NULL) {
		tprintf("FAILED: malloc(%u) succeeded\n", MEDIUMSIZE);
		return;
	}

	x = malloc(BIGSIZE);
	if (x != NULL) {
		tprintf("FAILED: malloc(%u) succeeded\n", BIGSIZE);
		return;
	}

	tprintf("Ok, now I'm going to free everything...\n");

	while (list != NULL) {
		tmp = list->next;

		if (checkblock(list->junk, sizeof(list->junk),
			       (uintptr_t)list, 0)) {
			failed = 1;
		}

		free(list);
		list = tmp;
	}

	if (failed) {
		tprintf("FAILED: data corruption\n");
		return;
	}

	tprintf("Let me see if I can allocate some more now...\n");

	x = malloc(MEDIUMSIZE);
	if (x == NULL) {
		tprintf("FAIL: Nope, I couldn't.\n");
		return;
	}
	free(x);

	tprintf("Passed malloc test 3\n");
}

////////////////////////////////////////////////////////////

/*
 * Test 4
 *
 * Tries to test in detail if malloc coalesces the free list properly.
 *
 * This test will likely fail if something other than a basic first-fit/
 * next-fit/best-fit algorithm is used.
 */

static
void
test4(void)
{
	void *x, *y, *z;
	unsigned long lx, ly, lz, overhead, zsize;

	tprintf("Entering malloc test 4.\n");
	tprintf("This test is intended for first/best-fit based mallocs.\n");
	tprintf("This test may not work correctly if run after other tests.\n");

	tprintf("Testing free list coalescing:\n");

	x = malloc(SMALLSIZE);
	if (x==NULL) {
		tprintf("FAILED: malloc(%u) failed\n", SMALLSIZE);
		return;
	}

	y = malloc(MEDIUMSIZE);
	if (y==NULL) {
		tprintf("FAILED: malloc(%u) failed\n", MEDIUMSIZE);
		return;
	}

	if (sizeof(unsigned long) < sizeof(void *)) {
		tprintf("Buh? I can't fit a void * in an unsigned long\n");
		tprintf("ENVIRONMENT FAILED...\n");
		return;
	}

	lx = (unsigned long)x;
	ly = (unsigned long)y;

	tprintf("x is 0x%lx; y is 0x%lx\n", lx, ly);

	/*
	 * The memory should look something like this:
	 *
	 *     OXXXOYYYYYYYYYYY
	 *
	 * where O are optional overhead (indexing) blocks.
	 */

	/* This is obviously wrong. */
	if (lx == ly) {
		tprintf("FAIL: x == y\n");
		return;
	}

	/*
	 * Check for overlap. It is sufficient to check if the start
	 * of each block is within the other block. (If the end of a
	 * block is within the other block, either the start is too,
	 * or the other block's start is within the first block.)
	 */
	if (lx < ly && lx + SMALLSIZE > ly) {
		tprintf("FAIL: y starts within x\n");
		return;
	}
	if (ly < lx && ly + MEDIUMSIZE > lx) {
		tprintf("FAIL: x starts within y\n");
		return;
	}

	/*
	 * If y is lower than x, some memory scheme we don't
	 * understand is in use, or else there's already stuff on the
	 * free list.
	 */
	if (ly < lx) {
		tprintf("TEST UNSUITABLE: y is below x\n");
		return;
	}

	/*
	 * Compute the space used by index structures.
	 */
	overhead = ly - (lx + SMALLSIZE);
	tprintf("Apparent block overhead: %lu\n", overhead);

	if (overhead > ABSURD_OVERHEAD) {
		tprintf("TEST UNSUITABLE: block overhead absurdly large.\n");
		return;
	}
	if (overhead > OVERHEAD) {
		tprintf("FAIL: block overhead is too large.\n");
		return;
	}

	tprintf("Freeing blocks...\n");
	free(x);
	free(y);

	zsize = SMALLSIZE + MEDIUMSIZE + overhead;

	tprintf("Now allocating %lu bytes... should reuse the space.\n", zsize);
	z = malloc(zsize);
	if (z == NULL) {
		tprintf("FAIL: Allocation failed...\n");
		return;
	}

	lz = (unsigned long) z;

	tprintf("z is 0x%lx (x was 0x%lx, y 0x%lx)\n", lz, lx, ly);

	if (lz==lx) {
		tprintf("Passed.\n");
	}
	else {
		tprintf("Failed.\n");
	}

	free(z);
}

////////////////////////////////////////////////////////////

/*
 * Test 5/6/7
 *
 * Generally beats on malloc/free.
 *
 * Test 5 uses random seed 0.
 * Test 6 seeds the random number generator from random:.
 * Test 7 asks for a seed.
 */

static
void
test567(int testno, unsigned long seed)
{
	static const int sizes[8] = { 13, 17, 69, 176, 433, 871, 1150, 6060 };

	void *ptrs[32];
	int psizes[32];
	int i, n, size, failed=0;

	srandom(seed);
	tprintf("Seeded random number generator with %lu.\n", seed);

	for (i=0; i<32; i++) {
		ptrs[i] = NULL;
		psizes[i] = 0;
	}

	for (i=0; i<100000; i++) {
		n = random()%32;
		if (ptrs[n] == NULL) {
			size = sizes[random()%8];
			ptrs[n] = malloc(size);
			psizes[n] = size;
			if (ptrs[n] == NULL) {
				tprintf("\nmalloc %u failed\n", size);
				failed = 1;
				break;
			}
			markblock(ptrs[n], size, n, 0);
		}
		else {
			size = psizes[n];
			if (checkblock(ptrs[n], size, n, 0)) {
				failed = 1;
				break;
			}
			free(ptrs[n]);
			ptrs[n] = NULL;
			psizes[n] = 0;
		}
		if (i%256==0) {
			tprintf(".");
		}
	}
	tprintf("\n");

	for (i=0; i<32; i++) {
		if (ptrs[i] != NULL) {
			free(ptrs[i]);
		}
	}

	if (failed) {
		tprintf("FAILED malloc test %d\n", testno);
	}
	else {
		tprintf("Passed malloc test %d\n", testno);
	}
}

static
void
test5(void)
{
	tprintf("Beginning malloc test 5\n");
	test567(5, 0);
}

static
void
test6(void)
{
	int fd, len;
	unsigned long seed;

	tprintf("Beginning malloc test 6\n");

	fd = open(_PATH_RANDOM, O_RDONLY);
	if (fd < 0) {
		err(1, "%s", _PATH_RANDOM);
	}
	len = read(fd, &seed, sizeof(seed));
	if (len < 0) {
		err(1, "%s", _PATH_RANDOM);
	}
	else if (len < (int)sizeof(seed)) {
		errx(1, "%s: Short read", _PATH_RANDOM);
	}
	close(fd);

	test567(6, seed);
}

static
void
test7(void)
{
	unsigned long seed;

	tprintf("Beginning malloc test 7\n");

	tprintf("Enter random seed: ");
	seed = geti();

	test567(7, seed);
}

////////////////////////////////////////////////////////////

static struct {
	int num;
	const char *desc;
	void (*func)(void);
} tests[] = {
	{ 1, "Simple allocation test", test1 },
	{ 2, "Allocate all memory in a big chunk", test2 },
	{ 3, "Allocate all memory in small chunks", test3 },
	{ 4, "Free list coalescing test (first/next/best-fit only)", test4 },
	{ 5, "Stress test", test5 },
	{ 6, "Randomized stress test", test6 },
	{ 7, "Stress test with particular seed", test7 },
	{ -1, NULL, NULL }
};

static
int
dotest(int tn)
{
	int i;
	for (i=0; tests[i].num>=0; i++) {
		if (tests[i].num == tn) {
			tests[i].func();
			return 0;
		}
	}
	return -1;
}

int
main(int argc, char *argv[])
{
	int i, tn, menu=1;

	if (argc > 1) {
		for (i=1; i<argc; i++) {
			dotest(atoi(argv[i]));
		}
		return 0;
	}

	while (1) {
		if (menu) {
			for (i=0; tests[i].num>=0; i++) {
				tprintf("  %2d  %s\n", tests[i].num,
				       tests[i].desc);
			}
			menu = 0;
		}
		tprintf("malloctest: ");
		tn = geti();
		if (tn < 0) {
			break;
		}

		if (dotest(tn)) {
			menu = 1;
		}
	}

	return 0;
}

