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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "extern.h"

static
void
randchar(char *c)
{
#if RAND_MAX != 0x7fffffff
#error "This code assumes RAND_MAX is 0x7fffffff"
#endif

	static long lbits = 0;
	static long lnum = 0;

	long bit;
	int ct = 0;

	*c = 0;

	while (ct < CHAR_BIT) {
		if (lnum==0) {
			lbits = random();
			lnum = 31;
		}

		bit = lbits & 1;
		if (bit) {
			(*c) |= 1;
		}
		(*c) <<= 1;
		ct++;
		lbits >>= 1;
		lnum--;
	}
}

static
void
fillrand(void *p, size_t len)
{
	size_t i;
	char *cp = p;
	for (i=0; i<len; i++) {
		randchar(&cp[i]);
	}
}

void *
randptr(void)
{
	void *x;
	fillrand(&x, sizeof(x));
	return x;
}

int
randint(void)
{
	int x;
	fillrand(&x, sizeof(x));
	return x;
}

off_t
randoff(void)
{
	off_t x;
	fillrand(&x, sizeof(x));
	return x;
}

size_t
randsize(void)
{
	size_t x;
	fillrand(&x, sizeof(x));
	return x;
}

static
void
usage(void)
{
	printf("Usage: randcall [-f] [-c count] [-r seed] 2|3|4|all\n");
	printf("   -f   suppress forking\n");
	printf("   -c   set iteration count (default 100)\n");
	printf("   -r   set pseudorandom seed (default 0)\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int count=100, seed = 0, dofork = 1;
	int an, i;

	for (i=1; i<argc; i++) {
		if (!strcmp(argv[i], "-f")) {
			dofork = 0;
		}
		else if (!strcmp(argv[i], "-c") && i<argc-1) {
			count = atoi(argv[++i]);
		}
		else if (!strcmp(argv[i], "-r") && i<argc-1) {
			seed = atoi(argv[++i]);
		}
		else if (argv[i][0] == '-') {
			usage();
		}
		else {
			break;
		}
	}
	if (i != argc-1) {
		usage();
	}

	if (!strcmp(argv[i], "all")) {
		an = 5;
	}
	else {
		an = atoi(argv[i]);
		if (an <2 || an > 4) {
			usage();
		}
	}

	printf("Seed: %d  Count: %d\n", seed, count);

	srandom(seed);
	trycalls(an, dofork, count);

	return 0;
}
