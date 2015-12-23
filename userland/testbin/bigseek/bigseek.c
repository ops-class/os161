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

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

/*
 * Test for seek positions > 2^32.
 *
 * This test is a bit hamstrung because neither emufs nor sfs supports
 * files larger than 2^32. (In fact, because sfs has 512-byte blocks,
 * to support a file of size 2^32 you'd need a quadruple-indirect
 * block, not just triple. But even with that it won't work because
 * the file size is a uint32_t.)
 *
 * We do, however, want to check if lseek is manipulating its 64-bit
 * argument correctly. The fs-independent code you're supposed to
 * write should be using off_t, which is 64 bits wide, to hold the
 * seek position. So seeking to positions past 2^32 should work, and
 * it should be possible to read back the seek position we set even if
 * it's past 2^32.
 *
 * Actually reading past 2^32 should behave the same as reading beyond
 * EOF anywhere else (produces EOF) and writing past 2^32 should yield
 * EFBIG.
 *
 * We also test between 2^31 and 2^32 in case values in that range are
 * getting truncated to a signed 32-bit value and then rejected for
 * being negative.
 */

#define TESTFILE "bigseekfile"

static const char *slogans[] = {
	"QUO USQUE TANDEM ABUTERE CATILINA PATENTIA NOSTRA",
	"QUEM IN FINEM SESE EFFRENATA IACTABIT AUDACIA"
};

static
void
write_slogan(int fd, unsigned which, bool failok)
{
	size_t len;
	ssize_t r;

	len = strlen(slogans[which]);
	r = write(fd, slogans[which], len);
	if (r < 0) {
		if (failok && errno == EFBIG) {
			return;
		}
		err(1, "write");
	}
	if (failok) {
		errx(1, "write: expected failure but wrote %zd bytes", r);
	}
	if ((size_t)r != len) {
		errx(1, "write: result %zd bytes, expected %zu", r, len);
	}
}

static
void
check_slogan(int fd, unsigned which)
{
	char buf[256];
	size_t len;
	ssize_t r;
	unsigned i, wrongcount;

	r = read(fd, buf, sizeof(buf));
	if (r < 0) {
		err(1, "read");
	}
	if (r == 0) {
		errx(1, "read: Unexpected EOF");
	}

	/* we should get either a full buffer or the length of the slogan */
	len = strlen(slogans[which]);
	if ((size_t)r != sizeof(buf) && (size_t)r != len) {
		errx(1, "read: result %zd bytes, expected %zu or %zu",
		     r, sizeof(buf), len);
	}

	/* slogan should match */
	if (memcmp(buf, slogans[which], len) != 0) {
		warnx("read: got wrong data");
		warnx("expected: %s", slogans[which]);
		buf[sizeof(buf) - 1] = 0;
		errx(1, "found: %s", buf);
	}

	/* bytes past the slogan (if any) should be 0 */
	wrongcount = 0;
	for (i=len; i<(size_t)r; i++) {
		if (buf[i] != 0) {
			warnx("read: buf[%zu] was 0x%x, expected 0", i,
			      (unsigned char)buf[i]);
			wrongcount++;
		}
	}
	if (wrongcount > 0) {
		errx(1, "%u bytes of trash in file", wrongcount);
	}
}

static
void
try_reading(int fd)
{
	char buf[16];
	ssize_t r;

	r = read(fd, buf, sizeof(buf));
	if (r == 0) {
		/* expected EOF */
		return;
	}
	if (r < 0) {
		err(1, "read");
	}
	errx(1, "read: Expected EOF but got %zd bytes", r);
}

static
void
try_writing(int fd)
{
	write_slogan(fd, 1, true);
}

static
void
dolseek(int fd, off_t pos, int whence, const char *whencestr, off_t expected)
{
	off_t result;

	result = lseek(fd, pos, whence);
	if (result == -1) {
		err(1, "lseek(fd, 0x%llx, %s)", pos, whencestr);
	}
	if (result != expected) {
		errx(1, "lseek(fd, 0x%llx, %s): Wrong return value"
		     " (got 0x%llx, expected 0x%llx)", pos, whencestr,
		     result, expected);
	}
}

static
void
try_seeking(int fd, off_t pos, off_t cursize)
{
	printf("Seeking to (and near) 0x%llx\n", pos);

	/* Go to the place. */
	dolseek(fd, pos, SEEK_SET, "SEEK_SET", pos);

	/* Go to where we already are. */
	dolseek(fd, 0, SEEK_CUR, "SEEK_CUR", pos);

	if (pos >= 10) {
		/* Back up a little. */
		dolseek(fd, -10, SEEK_CUR, "SEEK_CUR", pos - 10);

		/* Forward a little. */
		dolseek(fd, 20, SEEK_CUR, "SEEK_CUR", pos + 10);
	}
	else {
		/* Just forward a little. */
		dolseek(fd, 10, SEEK_CUR, "SEEK_CUR", pos + 10);
	}

	/* Via SEEK_END. */
	dolseek(fd, pos, SEEK_END, "SEEK_END", pos + cursize);

	/* Go back to the exact place. */
	dolseek(fd, pos, SEEK_SET, "SEEK_SET", pos);
}

int
main(void)
{
	off_t cursize;
	int fd;

	printf("Creating file...\n");
	fd = open(TESTFILE, O_RDWR|O_CREAT|O_TRUNC, 0664);
	if (fd < 0) {
		err(1, "%s", TESTFILE);
	}

	printf("Writing something at offset 0\n");
	write_slogan(fd, 0, false);
	cursize = strlen(slogans[0]);

	try_seeking(fd, (off_t)0x1000LL, cursize);

	printf("Writing something else\n");
	write_slogan(fd, 1, false);
	cursize = (off_t)0x1000LL + strlen(slogans[1]);

	try_seeking(fd, (off_t)0, cursize);

	/* If seek is totally bust, this will fail. */
	printf("Checking what we wrote\n");
	check_slogan(fd, 0);

	try_seeking(fd, (off_t)0x1000LL, cursize);
	printf("Checking the other thing we wrote\n");
	check_slogan(fd, 1);

	try_seeking(fd, (off_t)0x20LL, cursize);
	try_seeking(fd, (off_t)0x7fffffffLL, cursize);
	try_seeking(fd, (off_t)0x80000000LL, cursize);
	try_seeking(fd, (off_t)0x80000020LL, cursize);
	try_seeking(fd, (off_t)0x100000000LL, cursize);
	try_seeking(fd, (off_t)0x100000020LL, cursize);
	try_seeking(fd, (off_t)0x180000000LL, cursize);
	try_seeking(fd, (off_t)0x180000020LL, cursize);

	printf("Now trying to read (should get EOF)\n");
	try_reading(fd);

	printf("Now trying to write (should get EFBIG)\n");
	try_writing(fd);

	try_seeking(fd, (off_t)0x100000000LL, cursize);

	/* If seek truncates to 32 bits, this might read the slogan instead */
	printf("Trying to read again (should get EOF)\n");
	try_reading(fd);

	printf("Passed.\n");

	close(fd);
	remove(TESTFILE);
	return 0;
}
