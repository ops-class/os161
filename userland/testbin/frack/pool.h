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

struct poolctl {
	uint32_t *inuse;
	unsigned max;
	const char *itemtype;
	const char *maxname;
	const char *file;
};

#define DIVROUNDUP(a, b) 	(((a) + (b) - 1) / (b))
#define ROUNDUP(a, b) 		(DIVROUNDUP(a, b) * (b))

#define DECLPOOL(T, MAX) \
	static struct T pool_space_##T[ROUNDUP(MAX, 32)];	\
	static uint32_t pool_inuse_##T[DIVROUNDUP(MAX, 32)];	\
	static struct poolctl pool_##T = {			\
		.inuse = pool_inuse_##T,			\
		.max = ROUNDUP(MAX, 32),			\
		.itemtype = "struct " #T,			\
		.maxname = #MAX,				\
		.file = __FILE__				\
	}

#define POOLALLOC(T) (&pool_space_##T[poolalloc(&pool_##T)])
#define POOLFREE(T, x) (poolfree(&pool_##T, (x) - pool_space_##T))

unsigned poolalloc(struct poolctl *pool);
void poolfree(struct poolctl *pool, unsigned ix);
