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

#ifndef _KERN_TEST161_H_
#define _KERN_TEST161_H_

#define TEST161_SUCCESS 0
#define TEST161_FAIL 1

#include <kern/secret.h>

#ifdef _KERNEL
#define __TEST161_PROGRESS_N(iter, mod) do { \
	if (((iter) % mod) == 0) { \
		kprintf("."); \
	} \
} while (0)
#else
#include <stdio.h>
#define __TEST161_PROGRESS_N(iter, mod) do { \
	if (((iter) % mod) == 0) { \
		printf("."); \
	} \
} while (0)
#endif

// Always loud
#define TEST161_LPROGRESS_N(iter, mod) __TEST161_PROGRESS_N(iter, mod)
#define TEST161_LPROGRESS(iter) __TEST161_PROGRESS_N(iter, 100)

// Depends on whether or not it's automated testing. Some tests are
// quite verbose with useful information so these should just stay quiet.
#ifdef SECRET_TESTING
#define TEST161_TPROGRESS_N(iter, mod) __TEST161_PROGRESS_N(iter, mod)
#define TEST161_TPROGRESS(iter) __TEST161_PROGRESS_N(iter, 100)
#else
#define TEST161_TPROGRESS_N(iter, mod)
#define TEST161_TPROGRESS(iter)
#endif

int success(int, const char *, const char *);
int secprintf(const char *secret, const char *msg, const char *name);
int snsecprintf(size_t len, char *buffer, const char *secret, const char *msg, const char *name);
int partial_credit(const char *secret, const char *name, int scored, int total);

#ifdef _KERNEL
void test161_bootstrap(void);
#endif

#endif /* _KERN_TEST161_H_ */
