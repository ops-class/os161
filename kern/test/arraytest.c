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
#include <array.h>
#include <test.h>

#define TESTSIZE 73
#define BIGTESTSIZE 3000  /* more than one page of pointers */
#define NTH(i) ((void *)(0xb007 + 3*(i)))

static
void
testa(struct array *a)
{
	int testarray[TESTSIZE];
	int i, j, n, r, *p;

	for (i=0; i<TESTSIZE; i++) {
		testarray[i]=i;
	}

	n = array_num(a);
	KASSERT(n==0);

	for (i=0; i<TESTSIZE; i++) {
		r = array_add(a, &testarray[i], NULL);
		KASSERT(r==0);
		n = array_num(a);
		KASSERT(n==i+1);
	}
	n = array_num(a);
	KASSERT(n==TESTSIZE);

	for (i=0; i<TESTSIZE; i++) {
		p = array_get(a, i);
		KASSERT(*p == i);
	}
	n = array_num(a);
	KASSERT(n==TESTSIZE);

	for (j=0; j<TESTSIZE*4; j++) {
		i = random()%TESTSIZE;
		p = array_get(a, i);
		KASSERT(*p == i);
	}
	n = array_num(a);
	KASSERT(n==TESTSIZE);

	for (i=0; i<TESTSIZE; i++) {
		array_set(a, i, &testarray[TESTSIZE-i-1]);
	}

	for (i=0; i<TESTSIZE; i++) {
		p = array_get(a, i);
		KASSERT(*p == TESTSIZE-i-1);
	}

	r = array_setsize(a, TESTSIZE/2);
	KASSERT(r==0);

	for (i=0; i<TESTSIZE/2; i++) {
		p = array_get(a, i);
		KASSERT(*p == TESTSIZE-i-1);
	}

	array_remove(a, 1);

	for (i=1; i<TESTSIZE/2 - 1; i++) {
		p = array_get(a, i);
		KASSERT(*p == TESTSIZE-i-2);
	}
	p = array_get(a, 0);
	KASSERT(*p == TESTSIZE-1);

	array_setsize(a, 2);
	p = array_get(a, 0);
	KASSERT(*p == TESTSIZE-1);
	p = array_get(a, 1);
	KASSERT(*p == TESTSIZE-3);

	array_set(a, 1, NULL);
	array_setsize(a, 2);
	p = array_get(a, 0);
	KASSERT(*p == TESTSIZE-1);
	p = array_get(a, 1);
	KASSERT(p==NULL);

	array_setsize(a, TESTSIZE*10);
	p = array_get(a, 0);
	KASSERT(*p == TESTSIZE-1);
	p = array_get(a, 1);
	KASSERT(p==NULL);
}

int
arraytest(int nargs, char **args)
{
	struct array *a;

	(void)nargs;
	(void)args;

	kprintf("Beginning array test...\n");
	a = array_create();
	KASSERT(a != NULL);

	testa(a);

	array_setsize(a, 0);

	testa(a);

	array_setsize(a, 0);
	array_destroy(a);

	kprintf("Array test complete\n");
	return 0;
}

int
arraytest2(int nargs, char **args)
{
	struct array *a;
	void *p;
	unsigned i, x;
	int result;

	(void)nargs;
	(void)args;

	/* Silence warning with gcc 4.8 -Og (but not -O2) */
	x = 0;

	kprintf("Beginning large array test...\n");
	a = array_create();
	KASSERT(a != NULL);

	/* 1. Fill it one at a time. */
	p = (void *)0xc0ffee;
	for (i=0; i<BIGTESTSIZE; i++) {
		result = array_add(a, p, &x);
		KASSERT(result == 0);
		KASSERT(x == i);
	}
	KASSERT(array_num(a) == BIGTESTSIZE);

	/* 2. Check the contents */
	for (i=0; i<BIGTESTSIZE; i++) {
		KASSERT(array_get(a, i) == p);
	}

	/* 3. Clear it */
	result = array_setsize(a, 0);
	KASSERT(result == 0);

	/* 4. Set the size and initialize with array_set */
	result = array_setsize(a, BIGTESTSIZE);
	KASSERT(result == 0);
	for (i=0; i<BIGTESTSIZE; i++) {
		array_set(a, i, NTH(i));
	}

	/* 5. Check the contents again */
	for (i=0; i<BIGTESTSIZE; i++) {
		KASSERT(array_get(a, i) == NTH(i));
	}

	/* 6. Zot an entry and check the contents */
	array_remove(a, 1);
	KASSERT(array_get(a, 0) == NTH(0));
	KASSERT(array_num(a) == BIGTESTSIZE-1);
	for (i=1; i<BIGTESTSIZE-1; i++) {
		KASSERT(array_get(a, i) == NTH(i+1));
	}

	/* 7. Double the size and check the preexisting contents */
	result = array_setsize(a, BIGTESTSIZE*2);
	KASSERT(result == 0);
	KASSERT(array_get(a, 0) == NTH(0));
	for (i=1; i<BIGTESTSIZE-1; i++) {
		KASSERT(array_get(a, i) == NTH(i+1));
	}

	/* done */
	result = array_setsize(a, 0);
	KASSERT(result == 0);
	array_destroy(a);

	kprintf("Done.\n");

	return 0;
}
