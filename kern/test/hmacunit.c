/*
 * Copyright (c) 2015
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
#include <lib.h>
#include <spinlock.h>
#include <synch.h>
#include <thread.h>
#include <current.h>
#include <clock.h>
#include <test.h>
#include <kern/secure.h>

/*
 * Unit tests for hmac/sha256 hashing.
 */

////////////////////////////////////////////////////////////
// support code

static
void
ok(void)
{
	kprintf("Test passed.\n");
}

/*
 * Unit test 1
 *
 * Test some known msg/key/hashes to make sure we produce the
 * right results.
 */
static const char *plaintext1[] = {
	"The quick brown fox jumps over the lazy dog",
	"The only people for me are the mad ones",
	"I don't exactly know what I mean by that, but I mean it.",
};

static const char *keys1[] = {
	"xqWmgzbvGuLIeeKOrwMA",
	"ZxuvolLXL7C68pDjsclX",
	"PYeuVzKuB03awYDgJotS",
};

static const char *hashes1[] = {
	"251ab1da03c94435daf44898fcd11606669e222270e4ac90d04a18a9df8fdfd6",
	"75bbf48c53ccba08c244447ef7eff2e0a02f23acfdac6502282ec431823fb393",
	"6d7d2b5eabcda504f26de7547185483b19f9953a6eaeec6c364bb45e20b28598",
};

#define N_TESTS_1 3

int
hmacu1(int nargs, char **args)
{
	char *hash;
	int res;

	(void)nargs; (void)args;
	int i;
	for (i = 0; i < N_TESTS_1; i++)
	{
		res = hmac(plaintext1[i], strlen(plaintext1[i]), keys1[i], strlen(keys1[i]), &hash);
		KASSERT(!res);
		KASSERT(strcmp(hash, hashes1[i]) == 0);
		kfree(hash);
	}

	ok();
	/* clean up */
	return 0;
}
