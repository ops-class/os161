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

/* matmult.c
 *    Test program to do matrix multiplication on large arrays.
 *
 *    This version uses a storage-inefficient technique to get a
 *    shorter running time for the same memory usage.
 *
 *    Intended to stress virtual memory system.
 *
 *    Once the VM system assignment is complete your system should be
 *    able to survive this.
 */

#include <unistd.h>
#include <stdio.h>
#include <test161/test161.h>

#define Dim 	72	/* sum total of the arrays doesn't fit in
			 * physical memory
			 */

#define RIGHT  8772192		/* correct answer */

int A[Dim][Dim];
int B[Dim][Dim];
int C[Dim][Dim];
int T[Dim][Dim][Dim];

int
main(void)
{
    int i, j, k, r;

    for (i = 0; i < Dim; i++) {		/* first initialize the matrices */
		for (j = 0; j < Dim; j++) {
			TEST161_LPROGRESS_N(i*Dim + j, 1000);
			A[i][j] = i;
			B[i][j] = j;
			C[i][j] = 0;
		}
	}
	nprintf("\n");

    for (i = 0; i < Dim; i++) {		/* then multiply them together */
		for (j = 0; j < Dim; j++) {
            for (k = 0; k < Dim; k++) {
				TEST161_LPROGRESS_N(i*j*Dim*Dim + k, 50000);
				T[i][j][k] = A[i][k] * B[k][j];
			}
		}
	}
	nprintf("\n");

    for (i = 0; i < Dim; i++) {
		for (j = 0; j < Dim; j++) {
            for (k = 0; k < Dim; k++) {
				TEST161_LPROGRESS_N(i*j*Dim*Dim + k, 50000);
				C[i][j] += T[i][j][k];
			}
		}
	}
	nprintf("\n");

    r = 0;
    for (i = 0; i < Dim; i++)
	    r += C[i][i];

    nprintf("matmult finished.\n");
    nprintf("answer is: %d (should be %d)\n", r, RIGHT);
    if (r != RIGHT) {
	    nprintf("FAILED\n");
		success(TEST161_FAIL, SECRET, "/testbin/matmult");
	    return 1;
    }

    nprintf("Passed.\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/matmult");
    return 0;
}
