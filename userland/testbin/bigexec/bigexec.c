/*
 * Copyright (c) 2011
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
 * bigexec.c
 *
 * Checks that argv passing works and is not restricted to an
 * unreasonably small size.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <err.h>

#define _PATH_MYSELF "/testbin/bigexec"

////////////////////////////////////////////////////////////
// words

static const char word8[] = "Dalemark";
static char word4050[4051];
static char word16320[16321];
static char word65500[65501];

static
void
fill(char *buf, size_t buflen)
{
	static const char *const names[22] = {
		"Alhammitt", "Biffa", "Cennoreth", "Dastgandlen", "Enblith",
		"Fenna", "Gull", "Hern", "Hildrida", "Kankredin", "Kialan",
		"Lenina", "Manaliabrid", "Mayelbridwen", "Noreth", "Osfameron",
		"Robin", "Tanamil", "Tanamoril", "Tanaqui", "Ynen", "Ynynen"
	};

	const char *name;
	size_t len;

	while (buflen > 4) {
		name = names[random()%22];
		len = strlen(name);
		if (len < buflen) {
			strcpy(buf, name);
			buf += len;
			buflen -= len;
			if (buflen > 1) {
				*buf = ' ';
				buf++;
				buflen--;
			}
		}
	}
	while (buflen > 1) {
		*buf = '.';
		buf++;
		buflen--;
	}
	*buf = 0;
}

static
void
prepwords(void)
{
	srandom(16581);
	fill(word4050, sizeof(word4050));
	fill(word16320, sizeof(word16320));
	fill(word65500, sizeof(word65500));
}

////////////////////////////////////////////////////////////
// execing/checking

static
void
try(const char *first, ...)
{
	const char *args[20];
	const char *s;
	va_list ap;
	int num;

	assert(first != NULL);
	args[0] = _PATH_MYSELF;
	args[1] = first;
	num = 2;

	va_start(ap, first);
	while (1) {
		s = va_arg(ap, const char *);
		if (s == NULL) {
			break;
		}
		assert(num < 20);
		args[num++] = s;
	}
	assert(num < 20);
	args[num] = NULL;
	execv(_PATH_MYSELF, (char **)args);
	err(1, "execv");
}

static
void
trymany(int num, const char *word)
{
	const char *args[num+2];
	int i;

	args[0] = _PATH_MYSELF;
	for (i=0; i<num; i++) {
		args[i+1] = word;
	}
	args[num+1] = NULL;
	execv(_PATH_MYSELF, (char **)args);
	err(1, "execv");
}

static
int
check(int argc, char *argv[], const char *first, ...)
{
	const char *s;
	va_list ap;
	int pos;

	pos = 1;
	va_start(ap, first);
	s = first;
	while (s != NULL) {
		if (pos == argc) {
			/* not enough args */
			return 0;
		}
		assert(pos < argc);
		if (argv[pos] == NULL) {
			/* woops */
			warnx("argv[%d] is null but argc is %d", pos, argc);
			return 0;
		}
		if (strcmp(argv[pos], s) != 0) {
			/* does not match */
			return 0;
		}
		s = va_arg(ap, const char *);
		pos++;
	}
	if (pos != argc) {
		/* too many args */
		return 0;
	}
	if (argv[pos] != NULL) {
		warnx("argv[argc] is not null");
		return 0;
	}
	return 1;
}

static
int
checkmany(int argc, char *argv[], int num, const char *word)
{
	int i;

	if (argc != num + 1) {
		/* wrong number of args */
		return 0;
	}
	for (i=1; i<argc; i++) {
		if (strcmp(argv[i], word) != 0) {
			/* doesn't match */
			return 0;
		}
	}
	return 1;
}

////////////////////////////////////////////////////////////
// test driver

static
void
dumpargs(int argc, char *argv[])
{
	const char *s;
	int i;

	warnx("%d args", argc);
	warnx("argv[0]: %s", argv[0]);
	for (i=1; i<=argc; i++) {
		s = argv[i];
		if (s == NULL) {
			warnx("argv[%d]: is null", i);
		}
		else if (!strcmp(s, word8)) {
			warnx("argv[%d] is word8", i);
		}
		else if (!strcmp(s, word4050)) {
			warnx("argv[%d] is word4050", i);
		}
		else if (!strcmp(s, word16320)) {
			warnx("argv[%d] is word16320", i);
		}
		else if (!strcmp(s, word65500)) {
			warnx("argv[%d] is word65500", i);
		}
		else if (strlen(s) < 72) {
			warnx("argv[%d]: %s", i, s);
		}
		else {
			warnx("argv[%d] is %zu bytes, begins %.64s",
			      i, strlen(s), s);
		}
	}
}

int
main(int argc, char *argv[])
{
	if (argc < 0) {
		err(1, "argc is negative!?");
	}

	prepwords();
	assert(strlen(word8) == 8);
	assert(strlen(word4050) == 4050);
	assert(strlen(word16320) == 16320);
	assert(strlen(word65500) == 65500);

	assert(ARG_MAX >= 65536);

	if (argv == NULL || argc == 0 || argc == 1) {
		/* no args -- start the test */
		warnx("Starting.");

		/*
		 * 1. Should always fit no matter what.
		 */
		warnx("1. Execing with one 8-letter word.");
		try(word8, NULL);
	}
	else if (check(argc, argv, word8, NULL)) {
		/*
		 * 2. Fits in one page.
		 */
		warnx("2. Execing with one 4050-letter word.");
		try(word4050, NULL);
	}
	else if (check(argc, argv, word4050, NULL)) {
		/*
		 * 3. Requires two pages but each word fits on a page.
		 */
		warnx("3. Execing with two 4050-letter words.");
		try(word4050, word4050, NULL);
	}
	else if (check(argc, argv, word4050, word4050, NULL)) {
		/*
		 * 4. Requires the full 64K argv buffer, in large
		 * chunks, with a little space for slop. Each word
		 * fits on a page though. With null terminators and
		 * 4-byte pointers the size is 4085*16 = 65360, and
		 * with 8-byte pointers it would become 65424.
		 *
		 * Don't forget that argv[0] will be another 21 or 25
		 * bytes and some implementations may reasonably need
		 * to stash an ending NULL in the buffer too.
		 */
		warnx("4. Execing with 16 4050-letter words.");
		try(word4050, word4050, word4050, word4050,
		    word4050, word4050, word4050, word4050,
		    word4050, word4050, word4050, word4050,
		    word4050, word4050, word4050, word4050,
		    NULL);
	}
	else if (check(argc, argv,
		       word4050, word4050, word4050, word4050,
		       word4050, word4050, word4050, word4050,
		       word4050, word4050, word4050, word4050,
		       word4050, word4050, word4050, word4050,
		       NULL)) {
		/*
		 * 5. Requires more than one page for a single word.
		 */
		warnx("5. Execing with one 16320-letter word.");
		try(word16320, NULL);
	}
	else if (check(argc, argv, word16320, NULL)) {
		/*
		 * 6. Ditto but makes sure it works with two of them.
		 */
		warnx("6. Execing with two 16320-letter words.");
		try(word16320, word16320, NULL);
	}
	else if (check(argc, argv, word16320, word16320, NULL)) {
		/*
		 * 7. Requires the full 64K argv buffer.
		 */
		warnx("7. Execing with four 16320-letter words.");
		try(word16320, word16320, word16320, word16320,
		    NULL);
	}
	else if (check(argc, argv, word16320, word16320,
		       word16320, word16320, NULL)) {
		/*
		 * 8. Also requires the full 64K argv buffer, but with
		 * only one huge word.
		 */
		warnx("8. Execing with one 65500-letter word.");
		try(word65500, NULL);
	}
	else if (check(argc, argv, word65500, NULL)) {
		/*
		 * 9. This fits on one page. Given 4-byte pointers,
		 * (8+1+4)*300 = 3900. With 8-byte pointers, it
		 * doesn't, but we aren't doing that. (Update this if
		 * we ever move to a 64-bit platform.)
		 */
		assert((8+1+sizeof(char *))*300 < 4096);
		warnx("9. Execing with 300 8-letter words.");
		trymany(300, word8);
	}
	else if (checkmany(argc, argv, 300, word8)) {
#if 1 /* enforce the full size */
		/*
		 * 10. This requires the full 64K argv buffer.
		 * With 4-byte pointers, (8+1+4)*5020 = 65260.
		 * It also doesn't fit with 8-byte pointers.
		 *
		 * XXX for the time being, we'll allow less efficient
		 * implementations that use two pointers per word.
		 * Hence, (8+1+4+4)*3850 = 65450.
		 */
		assert((8+1+sizeof(char *))*5020 < 65536);
		assert((8+1+2*sizeof(char *))*3850 < 65536);
		warnx("10. Execing with 3850 8-letter words.");
		trymany(3850, word8);
	}
	else if (checkmany(argc, argv, 3850, word8)) {
#else
		/*
		 * 10a. This requires more than one page using small
		 * words. With 4-byte pointers, (8+1+4)*1000 = 13000.
		 */
		warnx("10. Execing with 1000 8-letter words.");
		trymany(1000, word8);
	}
	else if (checkmany(argc, argv, 1000, word8)) {
#endif
		warnx("Complete.");
		return 0;
	}
	else {
		warnx("Received unknown/unexpected args:");
		dumpargs(argc, argv);
		return 1;
	}
}
