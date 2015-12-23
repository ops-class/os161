/*
 * Copyright (c) 2015
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

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>

#include "tasks.h"

#define RIGHT 184621353

/*
 * comparison functions for qsort
 */
static
int
uintcmp(const void *av, const void *bv)
{
	unsigned a = *(const unsigned *)av;
	unsigned b = *(const unsigned *)bv;

	if (a < b) {
		return -1;
	}
	if (a > b) {
		return 1;
	}
	return 0;
}

static
int
altcmp(const void *av, const void *bv)
{
	unsigned a = *(const unsigned *)av;
	unsigned b = *(const unsigned *)bv;
	unsigned ax = (a & 0xffff0000) >> 16;
	unsigned ay = a & 0xffff;
	unsigned bx = (b & 0xffff0000) >> 16;
	unsigned by = b & 0xffff;

	if (ax < bx) {
		return 1;
	}
	if (ax > bx) {
		return -1;
	}
	if (ay < by) {
		return -1;
	}
	if (ay > by) {
		return 1;
	}
	return 0;
}

#if 0
/*
 * Shuffle an array.
 */
static
void
shuffle(unsigned *p, unsigned n)
{
	unsigned i, x, t;

	for (i=0; i<n; i++) {
		x = i + random() % (n-i);
		t = p[i];
		p[i] = p[x];
		p[x] = t;
	}
}
#endif

/*
 * Compute first differences.
 */
static
void
diffs(unsigned *p, unsigned n)
{
	unsigned p0;
	unsigned i;

	p0 = p[0];

	for (i=0; i<n-1; i++) {
		p[i] = p[i] - p[i+1];
	}
	p[n-1] = p[n-1] - p0;
}

/*
 * Take the sum.
 */
static
unsigned
sum(const unsigned *p, unsigned n)
{
	unsigned t, i;

	t = 0;
	for (i=0; i<n; i++) {
		t += p[i];
	}
	return t;
}

/*
 * grind - memory-bound task
 *
 * Note that this won't work until you have a VM system.
 */
void
grind(unsigned groupid, unsigned id)
{
	unsigned *p;
	unsigned i, n, s;

	(void)groupid;

	waitstart();

	/* each grind task uses 768K */
	n = (768*1024) / sizeof(*p);
	p = malloc(n * sizeof(*p));
	if (p == NULL) {
		if (errno == ENOSYS) {
			/*
			 * If we don't have sbrk, just bail out with
			 * "success" instead of failing the whole
			 * workload.
			 */
			errx(0, "grind: sbrk/malloc not implemented");
		}
		err(1, "malloc");
	}

	/* First, get some random integers. */
	warnx("grind %u: seeding", id);
	srandom(1753);
	for (i=0; i<n; i++) {
		p[i] = random();
	}

	/* Now sort them. */
	warnx("grind %u: sorting", id);
	qsort(p, n, sizeof(p[0]), uintcmp);

	/* Sort by a different comparison. */
	warnx("grind %u: sorting alternately", id);
	qsort(p, n, sizeof(p[0]), altcmp);

	/* Take the sum. */
	warnx("grind %u: summing", id);
	s = sum(p, n);
	warnx("grind %u: sum is %u (should be %u)", id, s, RIGHT);
	if (s != RIGHT) {
		errx(1, "grind %u FAILED", id);
	}

	/* Take first differences. */
	warnx("grind %u: first differences", id);
	diffs(p, n);

	/* Sort. */
	warnx("grind %u: sorting", id);
	qsort(p, n, sizeof(p[0]), uintcmp);

	warnx("grind %u: summing", id);
	s = sum(p, n);
	warnx("grind %u: sum is %u (should be 0)", id, s);
	if (s != 0) {
		errx(1, "grind %u FAILED", id);
	}
}
