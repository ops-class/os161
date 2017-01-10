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

/* sort.c
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Once the virtual memory assignment is complete, your system
 *    should survive this.
 */

#include <stdlib.h>
#include <string.h>
#include <err.h>

/* Larger than physical memory */
#define SIZE  (144*1024)


/*
 * Quicksort.
 *
 * This used to be a bubble sort, which was ok but slow in nachos with
 * 4k of memory and SIZE of 1024. However, with SIZE of 147,456 bubble
 * sort is completely unacceptable.
 *
 * Also, quicksort has somewhat more interesting memory usage patterns.
 */

static
void
sort(int *arr, int size)
{
	static int tmp[SIZE];
	int pivot, i, j, k;

	if (size<2) {
		return;
	}

	pivot = size/2;
	sort(arr, pivot);
	sort(&arr[pivot], size-pivot);

	i = 0;
	j = pivot;
	k = 0;
	while (i<pivot && j<size) {
		if (arr[i] < arr[j]) {
			tmp[k++] = arr[i++];
		}
		else {
			tmp[k++] = arr[j++];
		}
	}
	while (i<pivot) {
		tmp[k++] = arr[i++];
	}
	while (j<size) {
		tmp[k++] = arr[j++];
	}

	memcpy(arr, tmp, size*sizeof(int));
}

////////////////////////////////////////////////////////////

static int A[SIZE];

static
void
initarray(void)
{
	int i;

	/*
	 * Initialize the array, with pseudo-random but deterministic contents.
	 */
	srandom(533);

	for (i = 0; i < SIZE; i++) {
		A[i] = random();
	}
}

static
void
check(void)
{
	int i;

	for (i=0; i<SIZE-1; i++) {
		if (A[i] > A[i+1]) {
			errx(1, "Failed: A[%d] is %d, A[%d] is %d",
			     i, A[i], i+1, A[i+1]);
		}
	}
	warnx("Passed.");
}

int
main(void)
{
	initarray();
	sort(A, SIZE);
	check();
	return 0;
}
