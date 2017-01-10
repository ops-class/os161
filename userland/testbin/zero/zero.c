/*
 * Copyright (c) 2013
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
 * zero - check that the VM system zeros pages given to processes
 *
 * This program will be much more likely to detect a problem if you
 * run it *after* one of the out-of-core tests (huge, matmult, sort,
 * etc.)
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

/*
 * Some initialized data. This is here to increase the chance that
 * zeros[] spans page boundaries.
 */
static unsigned data_stuff[] = {
	1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
	2, 4, 6, 8, 0, 2, 4, 6, 8, 0, 2, 4, 6, 8, 0,
	1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
	2, 4, 6, 8, 0, 2, 4, 6, 8, 0, 2, 4, 6, 8, 0,
	1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
	2, 4, 6, 8, 0, 2, 4, 6, 8, 0, 2, 4, 6, 8, 0,
	1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
	2, 4, 6, 8, 0, 2, 4, 6, 8, 0, 2, 4, 6, 8, 0,
	1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5,
	2, 4, 6, 8, 0, 2, 4, 6, 8, 0, 2, 4, 6, 8, 0,
};

#define SUM_OF_DATA_STUFF 525

/*
 * Some uninitialized (BSS, zero) data. Make it more than one page
 * even if we happen to be on a machine with 8K pages.
 */
static unsigned bss_stuff[3000];

static
void
check_data(void)
{
	unsigned i, num, k;

	num = sizeof(data_stuff) / sizeof(data_stuff[0]);
	for (k=i=0; i<num; i++) {
		k += data_stuff[i];
	}
	if (k != SUM_OF_DATA_STUFF) {
		warnx("My initialized data sums to the wrong value!");
		warnx("Got: %u  Expected: %u", k, SUM_OF_DATA_STUFF);
		errx(1, "FAILED");
	}
}

static
void
check_bss(void)
{
	unsigned i, num;

	num = sizeof(bss_stuff) / sizeof(bss_stuff[0]);
	for (i=0; i<num; i++) {
		if (bss_stuff[i] != 0) {
			warnx("BSS entry at index %u (address %p) not zero!",
			      i, &bss_stuff[i]);
			warnx("Found: 0x%x", bss_stuff[i]);
			errx(1, "FAILED");
		}
	}
}

static
void
check_sbrk(void)
{
	char *base;
	unsigned i;

/* get at least one page, even if the page size is 8K */
#define SBRK_SIZE 8192

	base = sbrk(SBRK_SIZE);
	if (base == (void *)-1) {
		if (errno == ENOSYS) {
			printf("I guess you haven't implemented sbrk yet.\n");
			return;
		}
		err(1, "sbrk");
	}

	for (i=0; i<SBRK_SIZE; i++) {
		if (base[i] != 0) {
			warnx("Byte at offset %u (address %p) not zero",
			      i, &base[i]);
			warnx("Got: 0x%x", (unsigned char)base[i]);
			warnx("Base of sbrk region: %p", base);
			errx(1, "FAILED");
		}
	}
}


int
main(void)
{
	printf("zero: phase 1: checking .bss\n");
	check_data();
	check_bss();

	printf("zero: phase 2: checking sbrk()\n");
	check_sbrk();

	printf("zero: passed\n");
	return 0;
}
