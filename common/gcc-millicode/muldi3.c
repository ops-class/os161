/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * From:
 * 	@(#)muldi3.c	8.1 (Berkeley) 6/4/93
 *	NetBSD: muldi3.c,v 1.1 2005/12/20 19:28:51 christos Exp
 */

#include "longlong.h"

/*
 * Multiply two long longs.
 *
 * Our algorithm is based on the following.  Split incoming long long
 * values u and v (where u,v >= 0) into
 *
 *	u = 2^n u1  *  u0	(n = number of bits in `unsigned int', usu. 32)
 *
 * and
 *
 *	v = 2^n v1  *  v0
 *
 * Then
 *
 *	uv = 2^2n u1 v1  +  2^n u1 v0  +  2^n v1 u0  +  u0 v0
 *	   = 2^2n u1 v1  +     2^n (u1 v0 + v1 u0)   +  u0 v0
 *
 * Now add 2^n u1 v1 to the first term and subtract it from the middle,
 * and add 2^n u0 v0 to the last term and subtract it from the middle.
 * This gives:
 *
 *	uv = (2^2n + 2^n) (u1 v1)  +
 *	         (2^n)    (u1 v0 - u1 v1 + u0 v1 - u0 v0)  +
 *	       (2^n + 1)  (u0 v0)
 *
 * Factoring the middle a bit gives us:
 *
 *	uv = (2^2n + 2^n) (u1 v1)  +			[u1v1 = high]
 *		 (2^n)    (u1 - u0) (v0 - v1)  +	[(u1-u0)... = mid]
 *	       (2^n + 1)  (u0 v0)			[u0v0 = low]
 *
 * The terms (u1 v1), (u1 - u0) (v0 - v1), and (u0 v0) can all be done
 * in just half the precision of the original.  (Note that either or both
 * of (u1 - u0) or (v0 - v1) may be negative.)
 *
 * This algorithm is from Knuth vol. 2 (2nd ed), section 4.3.3, p. 278.
 *
 * Since C does not give us a `int * int = long long' operator, we split
 * our input long longs into two ints, then split the two ints into two
 * shorts.  We can then calculate `short * short = int' in native
 * arithmetic.
 *
 * Our product should, strictly speaking, be a `long long long', with
 * 128 bits, but we are going to discard the upper 64.  In other words,
 * we are not interested in uv, but rather in (uv mod 2^2n).  This
 * makes some of the terms above vanish, and we get:
 *
 *	(2^n)(high) + (2^n)(mid) + (2^n + 1)(low)
 *
 * or
 *
 *	(2^n)(high + mid + low) + low
 *
 * Furthermore, `high' and `mid' can be computed mod 2^n, as any factor
 * of 2^n in either one will also vanish.  Only `low' need be computed
 * mod 2^2n, and only because of the final term above.
 */
static long long __lmulq(unsigned int, unsigned int);

long long
__muldi3(long long a, long long b)
{
	union uu u, v, low, prod;
	unsigned int high, mid, udiff, vdiff;
	int negall, negmid;
#define	u1	u.ui[H]
#define	u0	u.ui[L]
#define	v1	v.ui[H]
#define	v0	v.ui[L]

	/*
	 * Get u and v such that u, v >= 0.  When this is finished,
	 * u1, u0, v1, and v0 will be directly accessible through the
	 * int fields.
	 */
	if (a >= 0)
		u.ll = a, negall = 0;
	else
		u.ll = -a, negall = 1;
	if (b >= 0)
		v.ll = b;
	else
		v.ll = -b, negall ^= 1;

	if (u1 == 0 && v1 == 0) {
		/*
		 * An (I hope) important optimization occurs when u1 and v1
		 * are both 0.  This should be common since most numbers
		 * are small.  Here the product is just u0*v0.
		 */
		prod.ll = __lmulq(u0, v0);
	} else {
		/*
		 * Compute the three intermediate products, remembering
		 * whether the middle term is negative.  We can discard
		 * any upper bits in high and mid, so we can use native
		 * unsigned int * unsigned int => unsigned int arithmetic.
		 */
		low.ll = __lmulq(u0, v0);

		if (u1 >= u0)
			negmid = 0, udiff = u1 - u0;
		else
			negmid = 1, udiff = u0 - u1;
		if (v0 >= v1)
			vdiff = v0 - v1;
		else
			vdiff = v1 - v0, negmid ^= 1;
		mid = udiff * vdiff;

		high = u1 * v1;

		/*
		 * Assemble the final product.
		 */
		prod.ui[H] = high + (negmid ? -mid : mid) + low.ui[L] +
		    low.ui[H];
		prod.ui[L] = low.ui[L];
	}
	return (negall ? -prod.ll : prod.ll);
#undef u1
#undef u0
#undef v1
#undef v0
}

/*
 * Multiply two 2N-bit ints to produce a 4N-bit long long, where N is
 * half the number of bits in an int (whatever that is---the code
 * below does not care as long as the header file does its part of the
 * bargain---but typically N==16).
 *
 * We use the same algorithm from Knuth, but this time the modulo refinement
 * does not apply.  On the other hand, since N is half the size of an int,
 * we can get away with native multiplication---none of our input terms
 * exceeds (UINT_MAX >> 1).
 *
 * Note that, for unsigned int l, the quad-precision (long long) result
 *
 *	l << N
 *
 * splits into high and low ints as HHALF(l) and LHUP(l) respectively.
 */
static long long
__lmulq(unsigned int u, unsigned int v)
{
	unsigned int u1, u0, v1, v0, udiff, vdiff, high, mid, low;
	unsigned int prodh, prodl, was;
	union uu prod;
	int neg;

	u1 = HHALF(u);
	u0 = LHALF(u);
	v1 = HHALF(v);
	v0 = LHALF(v);

	low = u0 * v0;

	/* This is the same small-number optimization as before. */
	if (u1 == 0 && v1 == 0)
		return (low);

	if (u1 >= u0)
		udiff = u1 - u0, neg = 0;
	else
		udiff = u0 - u1, neg = 1;
	if (v0 >= v1)
		vdiff = v0 - v1;
	else
		vdiff = v1 - v0, neg ^= 1;
	mid = udiff * vdiff;

	high = u1 * v1;

	/* prod = (high << 2N) + (high << N); */
	prodh = high + HHALF(high);
	prodl = LHUP(high);

	/* if (neg) prod -= mid << N; else prod += mid << N; */
	if (neg) {
		was = prodl;
		prodl -= LHUP(mid);
		prodh -= HHALF(mid) + (prodl > was);
	} else {
		was = prodl;
		prodl += LHUP(mid);
		prodh += HHALF(mid) + (prodl < was);
	}

	/* prod += low << N */
	was = prodl;
	prodl += LHUP(low);
	prodh += HHALF(low) + (prodl < was);
	/* ... + low; */
	if ((prodl += low) < low)
		prodh++;

	/* return 4N-bit product */
	prod.ui[H] = prodh;
	prod.ui[L] = prodl;
	return (prod.ll);
}
