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

/* matmult-orig.c
 *    Test program to do matrix multiplication on large arrays.
 *
 *    Intended to stress virtual memory system.
 *
 *    This is the original CS161 matmult program. Unfortunately,
 *    because matrix multiplication is order N^2 in space and N^3 in
 *    time, when this is made large enough to be an interesting VM
 *    test, it becomes so large that it takes hours to run.
 *
 *    So you probably want to just run matmult, which has been
 *    gimmicked up to be order N^3 in space and thus have a tolerable
 *    running time. This version is provided for reference only.
 *
 *    Once the VM assignment is complete your system should be able to
 *    survive this, if you have the patience to run it.
 */

#include <unistd.h>
#include <stdio.h>

#define Dim 	360	/* sum total of the arrays doesn't fit in
			 * physical memory
			 */

#define RIGHT  46397160		/* correct answer */

int A[Dim][Dim];
int B[Dim][Dim];
int C[Dim][Dim];

int
main(void)
{
    int i, j, k, r;

    for (i = 0; i < Dim; i++)		/* first initialize the matrices */
	for (j = 0; j < Dim; j++) {
	     A[i][j] = i;
	     B[i][j] = j;
	     C[i][j] = 0;
	}

    for (i = 0; i < Dim; i++)		/* then multiply them together */
	for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
		 C[i][j] += A[i][k] * B[k][j];

    tprintf("matmult-orig finished.\n");
    r = C[Dim-1][Dim-1];
    tprintf("answer is: %d (should be %d)\n", r, RIGHT);
    if (r != RIGHT) {
	    tprintf("FAILED\n");
    }
    else {
	    tprintf("Passed.\n");
    }
    return 0;
}
