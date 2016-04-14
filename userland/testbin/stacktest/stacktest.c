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
 * stacktest.c
 *
 * Tests the VM system's stack by allocating a large array on the stack and
 * accessing it in a sparse manner. In total, we allocate 4*200*4096 = 3.125M
 * on the stack. However, we only touch 1/4 of it, meaning this test should
 * run when with <=2M of memory if stack pages are allocated on demand.
 *
 * When the VM system assignment is done, your system should be able
 * to run this successfully.
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <test161/test161.h>

#define PageSize	4096
#define NumPages	200
#define Answer      4900

static int
stacktest1()
{
	int sparse[NumPages][PageSize];
	int i, j;

	for (i = 0; i < NumPages; i+=4) {
		// This is a fresh stack frame, so it better be zeroed. Otherwise,
		// the kernel is leaking information between processes.
		for (j = 0; j < PageSize/4; j++) {
			if (sparse[i][j] != 0) {
				errx(1, "Your stack pages are leaking data!");
			}
		}
		sparse[i][0] = i;
	}

	// We need to do something with the values so the compiler doesn't
	// optimize this array away.
	int total = 0;
	for (i = 0; i < NumPages; i+=4) {
		total += sparse[i][0];
	}

	return total;
}

int
main(void)
{
	int total = stacktest1();
	if (total != Answer) {
		errx(1, "Expected %d got %d\n", Answer, total);
	}

	// Success is not crashing
	success(TEST161_SUCCESS, SECRET, "/testbin/stacktest");
	return 0;
}
