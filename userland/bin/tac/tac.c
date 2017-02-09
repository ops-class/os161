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

/*
 * tac - print file backwards line by line (reverse cat)
 * usage: tac [files]
 *
 * This implementation copies the input to a scratch file, using a
 * second scratch file to keep notes, and then prints the scratch file
 * backwards. This is inefficient, but has the side effect of testing
 * the behavior of scratch files that have been unlinked.
 *
 * Note that if the remove system call isn't implemented, unlinking
 * the scratch files will fail and the scratch files will get left
 * behind. To avoid unnecessary noise (e.g. on emufs) we won't
 * complain about this.
 *
 * This program uses these system calls:
 *    getpid open read write lseek close remove _exit
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <err.h>

struct indexentry {
	off_t pos;
	off_t len;
};

static int datafd = -1, indexfd = -1;
static char dataname[64], indexname[64];

static char buf[4096];

////////////////////////////////////////////////////////////
// string ops

/* this is standard and should go into libc */
static
void *
memchr(const void *buf, int ch, size_t buflen)
{
	const unsigned char *ubuf = buf;
	size_t i;

	for (i=0; i<buflen; i++) {
		if (ubuf[i] == ch) {
			/* this must launder const */
			return (void *)(ubuf + i);
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////
// syscall wrappers

static
size_t
doread(int fd, const char *name, void *buf, size_t len)
{
	ssize_t r;

	r = read(fd, buf, len);
	if (r == -1) {
		err(1, "%s: read", name);
	}
	return (size_t)r;
}

static
void
dowrite(int fd, const char *name, const void *buf, size_t len)
{
	ssize_t r;

	r = write(fd, buf, len);
	if (r == -1) {
		err(1, "%s: write", name);
	}
	else if ((size_t)r != len) {
		errx(1, "%s: write: Unexpected short count %zd of %zu",
		     r, len);
	}
}

static
off_t
dolseek(int fd, const char *name, off_t pos, int whence)
{
	off_t ret;

	ret = lseek(fd, pos, whence);
	if (ret == -1) {
		err(1, "%s: lseek", name);
	}
	return ret;
}

////////////////////////////////////////////////////////////
// file I/O

static
void
readfile(const char *name)
{
	int fd, closefd;
	struct indexentry x;
	size_t len, remaining, here;
	const char *s, *t;
	
	if (name == NULL || !strcmp(name, "-")) {
		fd = STDIN_FILENO;
		closefd = -1;
	}
	else {
		fd = open(name, O_RDONLY);
		if (fd < 0) {
			err(1, "%s", name);
		}
		closefd = fd;
	}

	x.pos = 0;
	x.len = 0;
	while (1) {
		len = doread(fd, name, buf, sizeof(buf));
		if (len == 0) {
			break;
		}

		remaining = len;
		for (s = buf; s != NULL; s = t) {
			t = memchr(s, '\n', remaining);
			if (t != NULL) {
				t++;
				here = (t - s);
				x.len += here;
				remaining -= here;
				dowrite(indexfd, indexname, &x, sizeof(x));
				x.pos += x.len;
				x.len = 0;
			}
			else {
				x.len += remaining;
			}
		}
		dowrite(datafd, dataname, buf, len);
	}
	if (x.len > 0) {
		dowrite(indexfd, indexname, &x, sizeof(x));
	}

	if (closefd != -1) {
		close(closefd);
	}
}

static
void
dumpdata(void)
{
	struct indexentry x;
	off_t indexsize, pos, done;
	size_t amount, len;

	indexsize = dolseek(indexfd, indexname, 0, SEEK_CUR);
	pos = indexsize;
	assert(pos % sizeof(x) == 0);
	while (pos > 0) {
		pos -= sizeof(x);
		assert(pos >= 0);
		dolseek(indexfd, indexname, pos, SEEK_SET);

		len = doread(indexfd, indexname, &x, sizeof(x));
		if (len != sizeof(x)) {
			errx(1, "%s: read: Unexpected EOF", indexname);
		}
		dolseek(datafd, dataname, x.pos, SEEK_SET);

		for (done = 0; done < x.len; done += amount) {
			amount = sizeof(buf);
			if ((off_t)amount > x.len - done) {
				amount = x.len - done;
			}
			len = doread(datafd, dataname, buf, amount);
			if (len != amount) {
				errx(1, "%s: read: Unexpected short count"
				     " %zu of %zu", dataname, len, amount);
			}
			dowrite(STDOUT_FILENO, "stdout", buf, len);
		}
	}
}

////////////////////////////////////////////////////////////
// main

static
int
openscratch(const char *name, int flags, mode_t mode)
{
	int fd;

	fd = open(name, flags, mode);
	if (fd < 0) {
		err(1, "%s", name);
	}
	if (remove(name) < 0) {
		if (errno != ENOSYS) {
			err(1, "%s: remove", name);
		}
	}
	return fd;
}

static
void
openfiles(void)
{
	pid_t pid;

	pid = getpid();

	snprintf(dataname, sizeof(dataname), ".tmp.tacdata.%d", (int)pid);
	datafd = openscratch(dataname, O_RDWR|O_CREAT|O_TRUNC, 0664);

	snprintf(indexname, sizeof(indexname), ".tmp.tacindex.%d", (int)pid);
	indexfd = openscratch(indexname, O_RDWR|O_CREAT|O_TRUNC, 0664);
}

static
void
closefiles(void)
{
	close(datafd);
	close(indexfd);
	indexfd = datafd = -1;
}

int
main(int argc, char *argv[])
{
	int i;

	openfiles();

	if (argc > 1) {
		for (i=1; i<argc; i++) {
			readfile(argv[i]);
		}
	}
	else {
		readfile(NULL);
	}

	dumpdata();

	closefiles();
	return 0;
}
