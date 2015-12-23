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
#include <endian.h>

/*
 * Unconditional byte-swap functions.
 *
 * bswap16, 32, and 64 unconditionally swap byte order of integers of
 * the respective bitsize.
 *
 * The advantage of writing them out like this is that the bit
 * patterns are easily validated by inspection. Also, this form is
 * more likely to be picked up by the compiler and converted into
 * byte-swap machine instructions (if those exist) than something
 * loop-based.
 */

uint16_t
bswap16(uint16_t val)
{
	return    ((val & 0x00ff) << 8)
		| ((val & 0xff00) >> 8);
}

uint32_t
bswap32(uint32_t val)
{
	return    ((val & 0x000000ff) << 24)
		| ((val & 0x0000ff00) << 8)
		| ((val & 0x00ff0000) >> 8)
		| ((val & 0xff000000) >> 24);
}

uint64_t
bswap64(uint64_t val)
{
	return    ((val & 0x00000000000000ff) << 56)
		| ((val & 0x000000000000ff00) << 40)
		| ((val & 0x0000000000ff0000) << 24)
		| ((val & 0x00000000ff000000) << 8)
		| ((val & 0x000000ff00000000) << 8)
		| ((val & 0x0000ff0000000000) << 24)
		| ((val & 0x00ff000000000000) >> 40)
		| ((val & 0xff00000000000000) >> 56);
}

/*
 * Network byte order byte-swap functions.
 *
 * For ntoh* and hton*:
 *    *s are for "short" (16-bit)
 *    *l are for "long" (32-bit)
 *    *ll are for "long long" (64-bit)
 *
 * hton* convert from host byte order to network byte order.
 * ntoh* convert from network byte order to host byte order.
 *
 * Network byte order is big-endian.
 *
 * Note that right now the only platforms OS/161 runs on are
 * big-endian, so these functions are actually all empty.
 *
 * These should maybe be made inline.
 */

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define TO(tag, bits, type) \
    type ntoh##tag(type val) { return bswap##bits(val);	} \
    type hton##tag(type val) { return bswap##bits(val);	}
#endif

/*
 * Use a separate #if, so if the header file defining the symbols gets
 * omitted or messed up the build will fail instead of silently choosing
 * the wrong option.
 */
#if _BYTE_ORDER == _BIG_ENDIAN
#define TO(tag, bits, type) \
    type ntoh##tag(type val) { return val; } \
    type hton##tag(type val) { return val; }
#endif

#if _BYTE_ORDER == _PDP_ENDIAN
#error "You lose."
#endif

#ifndef TO
#error "_BYTE_ORDER not set"
#endif

TO(s,  16, uint16_t)
TO(l,  32, uint32_t)
TO(ll, 64, uint64_t)


/*
 * Some utility functions for handling 64-bit values.
 *
 * join32to64 pastes two adjoining 32-bit values together in the right
 * way to treat them as a 64-bit value, depending on endianness.
 * split64to32 is the inverse operation.
 *
 * The 32-bit arguments should be passed in the order they appear in
 * memory, not as high word and low word; the whole point of these
 * functions is to know which is the high word and which is the low
 * word.
 */

void
join32to64(uint32_t x1, uint32_t x2, uint64_t *y2)
{
#if _BYTE_ORDER == _BIG_ENDIAN
	*y2 = ((uint64_t)x1 << 32) | (uint64_t)x2;
#elif _BYTE_ORDER == _LITTLE_ENDIAN
	*y2 = (uint64_t)x1 | ((uint64_t)x2 << 32);
#else
#error "Eh?"
#endif
}

void
split64to32(uint64_t x, uint32_t *y1, uint32_t *y2)
{
#if _BYTE_ORDER == _BIG_ENDIAN
	*y1 = x >> 32;
	*y2 = x & 0xffffffff;
#elif _BYTE_ORDER == _LITTLE_ENDIAN
	*y1 = x & 0xffffffff;
	*y2 = x >> 32;
#else
#error "Eh?"
#endif
}
