/*
 * Copyright (c) 2014
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
#include <string.h>
#include <assert.h>

/*
 * qsort() for OS/161, where it isn't in libc.
 */
void
qsort(void *vdata, unsigned num, size_t size,
      int (*f)(const void *, const void *))
{
	unsigned pivot, head, tail;
	char *data = vdata;
	char tmp[size];

#define COMPARE(aa, bb) \
		((aa) == (bb) ? 0 : f(data + (aa) * size, data + (bb) * size))
#define EXCHANGE(aa, bb) \
		memcpy(tmp, data + (aa) * size, size);			\
		memcpy(data + (aa) * size, data + (bb) * size, size);	\
		memcpy(data + (bb) * size, tmp, size)


	if (num <= 1) {
		return;
	}
	if (num == 2) {
		if (COMPARE(0, 1) > 0) {
			EXCHANGE(0, 1);
			return;
		}
	}

	/*
	 * 1. Pick a pivot value. For simplicity, always use the
	 * middle of the array.
	 */
	pivot = num / 2;

	/*
	 * 2. Shift all values less than or equal to the pivot value
	 * to the front of the array.
	 */
	head = 0;
	tail = num - 1;

	while (head < tail) {
		if (COMPARE(head, pivot) <= 0) {
			head++;
		}
		else if (COMPARE(tail, pivot) > 0) {
			tail--;
		}
		else {
			EXCHANGE(head, tail);
			if (pivot == head) {
				pivot = tail;
			}
			else if (pivot == tail) {
				pivot = head;
			}
			head++;
			tail--;
		}
	}

	/*
	 * 3. If there's an even number of elements and we swapped the
	 * last two, the head and tail indexes will cross. In this
	 * case the first entry on the tail side is tail+1. If there's
	 * an odd number of elements, we stop with head == tail, and
	 * the first entry on the tail side is this value (hence,
	 * tail) if it's is greater than the pivot value, and the next
	 * element (hence, tail+1) if it's less than or equal to the
	 * pivot value.
	 *
	 * Henceforth use "tail" to hold the index of the first entry
	 * of the back portion of the array.
	 */
	if (head > tail || COMPARE(head, pivot) <= 0) {
		tail++;
	}

	/*
	 * 4. If we got a bad pivot that gave us only one partition,
	 * because of the order of the advances in the loop above it
	 * will always put everything in the front portion of the
	 * array (so tail == num). This happens if we picked the
	 * largest value. Move the pivot to the end, if necessary, lop
	 * off all values equal to it, and recurse on the rest.  (If
	 * there is no rest, the array is already sorted and we're
	 * done.)
	 */
	if (tail == num) {
		if (pivot < num - 1) {
			if (COMPARE(pivot, num - 1) > 0) {
				EXCHANGE(pivot, num - 1);
			}
		}
		tail = num - 1;
		while (tail > 0 && COMPARE(tail - 1, tail) == 0) {
			tail--;
		}
		if (tail > 0) {
			qsort(vdata, tail, size, f);
		}
		return;
	}
	assert(tail > 0 && tail < num);

	/*
	 * 5. Recurse on each subpart of the array.
	 */
	qsort(vdata, tail, size, f);
	qsort((char *)vdata + tail * size, num - tail, size, f);
}
