/*
 * Copyright (c) 2013
 *	The President and Fellows of Harvard College.
 *      Written by David A. Holland.
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

#include <stdint.h>
#include <assert.h>
#include <err.h>

#include "pool.h"

unsigned
poolalloc(struct poolctl *pool)
{
	uint32_t mask;
	unsigned j, i;

	assert(pool->max % 32 == 0);
	for (j=0; j<pool->max/32; j++) {
		for (mask=1, i=0; i<32; mask<<=1, i++) {
			if ((pool->inuse[j] & mask) == 0) {
				pool->inuse[j] |= mask;
				return j*32 + i;
			}
		}
	}
	errx(1, "Too many %s -- increase %s in %s",
	     pool->itemtype, pool->maxname, pool->file);
	return 0;
}

void
poolfree(struct poolctl *pool, unsigned num)
{
	uint32_t mask;
	unsigned pos;

	assert(num < pool->max);

	pos = num / 32;
	mask = 1 << (num % 32);

	assert(pool->inuse[pos] & mask);
	pool->inuse[pos] &= ~(uint32_t)mask;
}
