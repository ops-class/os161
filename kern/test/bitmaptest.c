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
#include <bitmap.h>
#include <test.h>

#define TESTSIZE 533

int
bitmaptest(int nargs, char **args)
{
	struct bitmap *b;
	char data[TESTSIZE];
	uint32_t x;
	int i;

	(void)nargs;
	(void)args;

	kprintf("Starting bitmap test...\n");

	for (i=0; i<TESTSIZE; i++) {
		data[i] = random()%2;
	}

	b = bitmap_create(TESTSIZE);
	KASSERT(b != NULL);

	for (i=0; i<TESTSIZE; i++) {
		KASSERT(bitmap_isset(b, i)==0);
	}

	for (i=0; i<TESTSIZE; i++) {
		if (data[i]) {
			bitmap_mark(b, i);
		}
	}
	for (i=0; i<TESTSIZE; i++) {
		if (data[i]) {
			KASSERT(bitmap_isset(b, i));
		}
		else {
			KASSERT(bitmap_isset(b, i)==0);
		}
	}

	for (i=0; i<TESTSIZE; i++) {
		if (data[i]) {
			bitmap_unmark(b, i);
		}
		else {
			bitmap_mark(b, i);
		}
	}
	for (i=0; i<TESTSIZE; i++) {
		if (data[i]) {
			KASSERT(bitmap_isset(b, i)==0);
		}
		else {
			KASSERT(bitmap_isset(b, i));
		}
	}

	while (bitmap_alloc(b, &x)==0) {
		KASSERT(x < TESTSIZE);
		KASSERT(bitmap_isset(b, x));
		KASSERT(data[x]==1);
		data[x] = 0;
	}

	for (i=0; i<TESTSIZE; i++) {
		KASSERT(bitmap_isset(b, i));
		KASSERT(data[i]==0);
	}

	kprintf("Bitmap test complete\n");
	return 0;
}
