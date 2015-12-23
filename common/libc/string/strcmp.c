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
 * This file is shared between libc and the kernel, so don't put anything
 * in here that won't work in both contexts.
 */

#ifdef _KERNEL
#include <types.h>
#include <lib.h>
#else
#include <string.h>
#endif

/*
 * Standard C string function: compare two strings and return their
 * sort order.
 */

int
strcmp(const char *a, const char *b)
{
	size_t i;

	/*
	 * Walk down both strings until either they're different
	 * or we hit the end of A.
	 *
	 * If A and B strings are not the same length, when the
	 * shorter one ends, the two will be different, and we'll
	 * stop before running off the end of either.
	 *
	 * If they *are* the same length, it's sufficient to check
	 * that we haven't run off the end of A, because that's the
	 * same as checking to make sure we haven't run off the end of
	 * B.
	 */

	for (i=0; a[i]!=0 && a[i]==b[i]; i++) {
		/* nothing */
	}

	/*
	 * If A is greater than B, return 1. If A is less than B,
	 * return -1.  If they're the same, return 0. Since we have
	 * stopped at the first character of difference (or the end of
	 * both strings) checking the character under I accomplishes
	 * this.
	 *
	 * Note that strcmp does not handle accented characters,
	 * internationalization, or locale sort order; strcoll() does
	 * that.
	 *
	 * The rules say we compare order in terms of *unsigned* char.
	 */
	if ((unsigned char)a[i] > (unsigned char)b[i]) {
		return 1;
	}
	else if (a[i] == b[i]) {
		return 0;
	}
	return -1;
}
