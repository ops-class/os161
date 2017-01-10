/*
 * bloat - waste memory.
 *
 * This test allocates memory a page at a time and keeps going until
 * it runs out. It gets the memory directly with sbrk to avoid malloc-
 * related overheads, which as long as OS/161 has a dumb userlevel
 * malloc is important for performance.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <err.h>

/* OS/161 doesn't currently have a way to get this from the kernel. */
#define PAGE_SIZE 4096

/* the memory we've gotten */
static void *firstpage;
static void *lastpage;

/* number of page allocations per cycle */
static unsigned allocs;

/* number of pages to touch every cycle */
static unsigned touchpages;

/* when touching pages, the extent to which we favor the middle of the range */
static unsigned bias;


static
void
moremem(void)
{
	static unsigned totalpages;

	void *ptr;
	unsigned i;

	for (i=0; i<allocs; i++) {
		ptr = sbrk(PAGE_SIZE);
		if (ptr == (void *)-1) {
			err(1, "After %u pages: sbrk", totalpages);
		}
		totalpages++;
		lastpage = ptr;
		if (firstpage == NULL) {
			firstpage = ptr;
		}
	}
}

static
void
touchpage(unsigned pagenum)
{
	int *ptr;

	ptr = (void *)((uintptr_t)firstpage + PAGE_SIZE * pagenum);
	*ptr = pagenum;
}

static
unsigned
pickpage(unsigned numpages)
{
	unsigned mnum, moffset;
	unsigned span, val, i;

	/* take 1 in 1000 pages uniformly from the entire space */
	if (random() % 1000 == 0) {
		return random() % numpages;
	}

	/* the rest is taken from the middle 1% */

	mnum = numpages / 100;
	if (mnum < touchpages * 2) {
		mnum = touchpages * 2;
	}
	if (mnum >= numpages) {
		mnum = numpages;
	}
	moffset = numpages / 2 - mnum / 2;

	assert(bias >= 1);
	span = (mnum + bias - 1) / bias;

	do {
		val = 0;
		for (i=0; i<bias; i++) {
			val += random() % span;
		}
	} while (val >= mnum);
	return moffset + val;
}

static
void
touchmem(void)
{
	unsigned i, num;

	num = (((uintptr_t)lastpage - (uintptr_t)firstpage) / PAGE_SIZE) + 1;

	if (num % 256 == 0) {
		warnx("%u pages", num);
	}

	for (i=0; i<touchpages; i++) {
		touchpage(pickpage(num));
	}
}

static
void
run(void)
{
	while (1) {
		moremem();
		touchmem();
	}
}

static
void
printsettings(void)
{
	printf("Page size: %u\n", PAGE_SIZE);
	printf("Allocating %u pages and touching %u pages on each cycle.\n",
	       allocs, touchpages);
	printf("Page selection bias: %u\n", bias);
	printf("\n");
}

static
void
usage(void)
{
	warnx("bloat [-a allocs] [-b bias] [-p pages]");
	warnx("   allocs: number of pages allocated per cycle (default 4)");
	warnx("   bias: number of dice rolled to touch pages (default 8)");
	warnx("   pages: pages touched per cycle (default 8)");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int i;

	/* default mode */
	allocs = 4;
	touchpages = 8;
	bias = 8;

	srandom(1234);

	for (i=1; i<argc; i++) {
		if (!strcmp(argv[i], "-a")) {
			i++;
			if (i == argc) {
				errx(1, "-a: option requires argument");
			}
			allocs = atoi(argv[i]);
			if (allocs == 0) {
				errx(1, "-a: must not be zero");
			}
		}
		else if (!strcmp(argv[i], "-b")) {
			i++;
			if (i == argc) {
				errx(1, "-b: option requires argument");
			}
			bias = atoi(argv[i]);
			if (bias == 0) {
				errx(1, "-b: must not be zero");
			}
		}
		else if (!strcmp(argv[i], "-h")) {
			usage();
		}
		else if (!strcmp(argv[i], "-p")) {
			i++;
			if (i == argc) {
				errx(1, "-p: option requires argument");
			}
			touchpages = atoi(argv[i]);
		}
		else {
			errx(1, "Argument %s not recognized", argv[i]);
			usage();
		}
	}

	printsettings();
	run();
	return 0;
}
