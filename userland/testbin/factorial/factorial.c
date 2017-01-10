/*
 * Copyright (c) 2014
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <err.h>

#define _PATH_SELF "/testbin/factorial"

/*
 * factorial - compute factorials by recursive exec
 *
 * External usage: factorial N
 * (compute N!)
 *
 * Internal usage: factoral N M
 * (compute N! * M)
 */

////////////////////////////////////////////////////////////
// arithmetic

/*
 * We compute using binary-coded decimal integers where
 * each byte contains the digit characters '0' through '9'.
 * This is not exactly optimum for computation but it's
 * perfect for sending numbers through execv.
 */

#define NUMSIZE 8191
struct number {
	char buf[NUMSIZE+1];	/* includes space for a null-terminator */
	size_t first;		/* first valid digit */
};

static struct number scratch;

static
void
number_init(struct number *n, const char *txt)
{
	size_t len, i;

	len = strlen(txt);
	if (len > NUMSIZE) {
		warnx("%s", txt);
		errx(1, "Number too large");
	}
	n->first = NUMSIZE - len;
	strcpy(n->buf + n->first, txt);
#if 0
	for (i=0; i<n->first; i++) {
		n->buf[i] = '0';
	}
#endif
	for (i=n->first; i<NUMSIZE; i++) {
		if (n->buf[i] < '0' || n->buf[i] > '9') {
			warnx("%s", txt);
			errx(1, "Number contained non-digit characters");
		}
	}
	assert(n->buf[NUMSIZE] == 0);
	while (n->first < NUMSIZE && n->buf[n->first] == '0') {
		n->first++;
	}
}

static
char *
number_get(struct number *n)
{
	size_t pos;

	pos = n->first;
	while (pos < NUMSIZE && n->buf[pos] == '0') {
		pos++;
	}
	if (pos == NUMSIZE) {
		pos--;
		n->buf[pos] = '0';
	}
	return &n->buf[pos];
}

static
void
finishcarry(struct number *r, const struct number *b, size_t pos,
	    unsigned carry)
{
	if (carry > 0 && b->first == 0) {
		/* if b->first is 0, pos may now be 2^32-1 */
		errx(1, "Overflow");
	}
	while (carry > 0) {
		if (pos == 0) {
			errx(1, "Overflow");
		}
		r->buf[pos--] = carry % 10 + '0';
		carry = carry / 10;
	}
	r->first = pos + 1;
}

static
void
pluseq(struct number *r, const struct number *b)
{
	size_t pos;
	unsigned an, bn, rn, carry;

	carry = 0;
	for (pos = NUMSIZE; pos-- > b->first; ) {
		an = pos < r->first ? 0 : r->buf[pos] - '0';
		bn = b->buf[pos] - '0';
		rn = an + bn + carry;
		r->buf[pos] = rn % 10 + '0';
		carry = rn / 10;
	}
	finishcarry(r, b, pos, carry);
}

static
void
dec(struct number *r)
{
	size_t pos;

	for (pos = NUMSIZE; pos-- > r->first; ) {
		if (r->buf[pos] == '0') {
			r->buf[pos] = '9';
		}
		else {
			r->buf[pos]--;
			return;
		}
	}
	/* This should really not happen. */
	errx(1, "Underflow");
}

static
void
multc(struct number *r, const struct number *a, unsigned bn, size_t offset)
{
	size_t pos;
	unsigned an, rn, carry;

	for (pos = NUMSIZE; pos-- > NUMSIZE - offset; ) {
		r->buf[pos] = '0';
	}
	carry = 0;
	for (pos = NUMSIZE; pos-- > a->first; ) {
		an = a->buf[pos] - '0';
		rn = an * bn + carry;
		if (pos < offset) {
			errx(1, "Overflow");
		}
		r->buf[pos - offset] = rn % 10 + '0';
		carry = rn / 10;
	}
	if (carry > 0 && pos < offset) {
		errx(1, "Overflow");
	}
	finishcarry(r, a, pos - offset, carry);
}

static
void
mult(struct number *r, const struct number *a, const struct number *b)
{
	unsigned offset;
	size_t apos;

	/* B should normally be the larger number */
	if (a->first < b->first) {
		mult(r, b, a);
		return;
	}

	number_init(&scratch, "0");
	offset = 0;
	for (apos = NUMSIZE; apos-- > a->first; ) {
		multc(&scratch, b, a->buf[apos] - '0', offset);
		pluseq(r, &scratch);
		offset++;
	}
}

////////////////////////////////////////////////////////////
// argv logic

static
void
self(const char *arg1, const char *arg2)
{
	const char *args[4];

	args[0] = _PATH_SELF;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = NULL;
	execv(_PATH_SELF, (char **)args);
	err(1, "execv");
}

static struct number n1, n2, multbuf;

int
main(int argc, char *argv[])
{
	if (argc == 0) {
		/* Assume we've just been run from the menu. */
		self("404", "1");
	}
	else if (argc == 2) {
		self(argv[1], "1");
	}
	else if (argc == 3) {
		if (!strcmp(argv[1], "1") || !strcmp(argv[1], "0")) {
			printf("%s\n", argv[2]);
		}
		else {
			number_init(&n1, argv[1]);
			number_init(&n2, argv[2]);
			number_init(&multbuf, "0");
			mult(&multbuf, &n1, &n2);
			dec(&n1);
			self(number_get(&n1), number_get(&multbuf));
		}
	}
	else {
		warnx("Usage: factorial N");
	}
	return 0;
}
