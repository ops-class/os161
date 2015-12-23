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
#include <stdlib.h>
#include <string.h>
#endif

/*
 * Standard C function: parse a string that represents a decimal integer.
 * Leading whitespace is allowed. Trailing gunk is allowed too. Doesn't
 * really report syntax errors or overflow in any useful way.
 */

int
atoi(const char *s)
{
	static const char digits[] = "0123456789";  /* legal digits in order */
	unsigned val=0;		/* value we're accumulating */
	int neg=0;		/* set to true if we see a minus sign */

	/* skip whitespace */
	while (*s==' ' || *s=='\t') {
		s++;
	}

	/* check for sign */
	if (*s=='-') {
		neg=1;
		s++;
	}
	else if (*s=='+') {
		s++;
	}

	/* process each digit */
	while (*s) {
		const char *where;
		unsigned digit;

		/* look for the digit in the list of digits */
		where = strchr(digits, *s);
		if (where==NULL) {
			/* not found; not a digit, so stop */
			break;
		}

		/* get the index into the digit list, which is the value */
		digit = (where - digits);

		/* could (should?) check for overflow here */

		/* shift the number over and add in the new digit */
		val = val*10 + digit;

		/* look at the next character */
		s++;
	}

	/* handle negative numbers */
	if (neg) {
		return -val;
	}

	/* done */
	return val;
}
