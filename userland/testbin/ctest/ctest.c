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
 * Performance test from former 161 prof. Brad Chen
 * Stresses VM.
 *
 * Intended for the VM assignment. This should run successfully on a
 * variety of strides when the VM system is complete. Strides that are
 * not a multiple of 2 work better; see below.
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * SIZE is the amount of memory used.
 * DEFAULT is the default stride.
 * Note that SIZE and DEFAULT should be relatively prime.
 */
#define SIZE      (1024*1024/sizeof(struct entry))
#define DEFAULT   477

struct entry {
	struct entry *e;
};

struct entry array[SIZE];

int
main(int argc, char **argv)
{
	volatile struct entry *e;
	unsigned i, stride;

	stride = DEFAULT;
	if (argc == 2) {
		stride = atoi(argv[1]);
	}
	if (stride <= 0 || argc > 2) {
		printf("Usage: ctest [stridesize]\n");
		printf("   stridesize should not be a multiple of 2.\n");
		return 1;
	}

	printf("Starting ctest: stride %d\n", stride);

	/*
	 * Generate a huge linked list, with each entry pointing to
	 * the slot STRIDE entries above it. As long as STRIDE and SIZE
	 * are relatively prime, this will put all the entries on one
	 * list. Otherwise you will get multiple disjoint lists. (All
	 * these lists will be circular.)
	 */
	for (i=0; i<SIZE; i++) {
		array[i].e = &array[(i+stride) % SIZE];
	}

	/*
	 * Traverse the list. We stop after hitting each element once.
	 *
	 * (If STRIDE was even, this will hit some elements more than
	 * once and others not at all.)
	 */
	e = &array[0];
	for (i=0; i<SIZE; i++) {
		if (i % stride == 0) {
			putchar('.');
		}
		e = e->e;
	}

	printf("\nDone!\n");
	return 0;
}
