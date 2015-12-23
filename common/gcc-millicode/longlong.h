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
 *	@(#)quad.h	8.1 (Berkeley) 6/4/93
 *	NetBSD: quad.h,v 1.1 2005/12/20 20:29:40 christos Exp
 */

/*
 * Long long arithmetic.
 *
 * This library makes the following assumptions:
 *
 *  - The type long long exists.
 *
 *  - A long long variable is exactly twice as long as `int'.
 *
 *  - The machine's arithmetic is two's complement.
 *
 * This library can provide 128-bit arithmetic on a machine with
 * 128-bit long longs and 64-bit ints, for instance, or 96-bit
 * arithmetic on machines with 48-bit ints.
 *
 * The names are built into gcc.
 */

#if defined(_KERNEL)
#include <types.h>
#include <endian.h>
#else
#include <sys/types.h>
#include <sys/endian.h>
#endif

#include <limits.h>

/*
 * Depending on the desired operation, we view a `long long' in
 * one or more of the following formats.
 */
union uu {
	long long          ll;		/* as a (signed) long long */
	unsigned long long ull;		/* as an unsigned long long */
	int                si[2];	/* as two (signed) ints */
	unsigned int       ui[2];	/* as two unsigned ints */
};

/*
 * Define high and low parts of a long long.
 */
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define H 1
#define L 0
#endif

#if _BYTE_ORDER == _BIG_ENDIAN
#define H 0
#define L 1
#endif


/*
 * Total number of bits in a long long and in the pieces that make it up.
 * These are used for shifting, and also below for halfword extraction
 * and assembly.
 */
#define	LONGLONG_BITS	(sizeof(long long) * CHAR_BIT)
#define	INT_BITS	(sizeof(int) * CHAR_BIT)
#define	HALF_BITS	(sizeof(int) * CHAR_BIT / 2)

/*
 * Extract high and low shortwords from longword, and move low shortword of
 * longword to upper half of long, i.e., produce the upper longword of
 * ((long long)(x) << (number_of_bits_in_int/2)).
 * [`x' must actually be unsigned int.]
 *
 * These are used in the multiply code, to split a longword into upper
 * and lower halves, and to reassemble a product as a long long, shifted
 * left (sizeof(int)*CHAR_BIT/2).
 */
#define	HHALF(x)	((unsigned int)(x) >> HALF_BITS)
#define	LHALF(x)	((unsigned int)(x) & (((int)1 << HALF_BITS) - 1))
#define	LHUP(x)		((unsigned int)(x) << HALF_BITS)

long long          __adddi3      (         long long,          long long);
long long          __anddi3      (         long long,          long long);
long long          __ashldi3     (         long long, unsigned int);
long long          __ashrdi3     (         long long, unsigned int);
int                __cmpdi2      (         long long,          long long);
long long          __divdi3      (         long long,          long long);
long long          __iordi3      (         long long,          long long);
long long          __lshldi3     (         long long, unsigned int);
long long          __lshrdi3     (         long long, unsigned int);
long long          __moddi3      (         long long,          long long);
long long          __muldi3      (         long long,          long long);
long long          __negdi2      (         long long);
long long          __one_cmpldi2 (         long long);
long long          __subdi3      (         long long,          long long);
int                __ucmpdi2     (unsigned long long, unsigned long long);
unsigned long long __udivdi3     (unsigned long long, unsigned long long);
unsigned long long __umoddi3     (unsigned long long, unsigned long long);
long long          __xordi3      (         long long,          long long);

#ifndef _KERNEL
long long          __fixdfdi     (double);
long long          __fixsfdi     (float);
unsigned long long __fixunsdfdi  (double);
unsigned long long __fixunssfdi  (float);
double             __floatdidf   (long long);
float              __floatdisf   (long long);
double             __floatunsdidf(unsigned long long);
#endif

unsigned long long __qdivrem     (unsigned long long, unsigned long long,
				  unsigned long long *);
