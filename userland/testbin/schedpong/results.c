/*
 * Copyright (c) 2015
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

#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <assert.h>

#include "results.h"

#define RESULTSFILE "endtimes"

static int resultsfile = -1;

/*
 * Create the file that the timing results are written to.
 * This is done first, in the main process.
 */
void
createresultsfile(void)
{
	int fd;

	assert(resultsfile == -1);

	fd = open(RESULTSFILE, O_RDWR|O_CREAT|O_TRUNC, 0664);
	if (fd < 0) {
		err(1, "%s", RESULTSFILE);
	}
	if (close(fd) == -1) {
		warn("%s: close", RESULTSFILE);
	}
}

/*
 * Remove the timing results file.
 * This is done last, in the main process.
 */
void
destroyresultsfile(void)
{
	if (remove(RESULTSFILE) == -1) {
		if (errno != ENOSYS) {
			warn("%s: remove", RESULTSFILE);
		}
	}
}

/*
 * Open the timing results file. This is done separately for writing
 * in each process to avoid sharing the seek position (which would
 * then require extra semaphoring to coordinate...) and afterwards
 * done for reading in the main process.
 */
void
openresultsfile(int openflags)
{
	assert(openflags == O_RDONLY || openflags == O_WRONLY);
	assert(resultsfile == -1);

	resultsfile = open(RESULTSFILE, openflags, 0);
	if (resultsfile < 0) {
		err(1, "%s", RESULTSFILE);
	}
}

/*
 * Close the timing results file.
 */
void
closeresultsfile(void)
{
	assert(resultsfile >= 0);

	if (close(resultsfile) == -1) {
		warn("%s: close", RESULTSFILE);
	}
	resultsfile = -1;
}

/*
 * Write a result into the timing results file.
 */
void
putresult(unsigned groupid, time_t secs, unsigned long nsecs)
{
	off_t pos;
	ssize_t r;

	assert(resultsfile >= 0);

	pos = groupid * (sizeof(secs) + sizeof(nsecs));
	if (lseek(resultsfile, pos, SEEK_SET) == -1) {
		err(1, "%s: lseek", RESULTSFILE);
	}
	r = write(resultsfile, &secs, sizeof(secs));
	if (r < 0) {
		err(1, "%s: write (seconds)", RESULTSFILE);
	}
	if ((size_t)r < sizeof(secs)) {
		errx(1, "%s: write (seconds): Short write", RESULTSFILE);
	}
	r = write(resultsfile, &nsecs, sizeof(nsecs));
	if (r < 0) {
		err(1, "%s: write (nsecs)", RESULTSFILE);
	}
	if ((size_t)r < sizeof(nsecs)) {
		errx(1, "%s: write (nsecs): Short write", RESULTSFILE);
	}
}

/*
 * Read a result from the timing results file.
 */
void
getresult(unsigned groupid, time_t *secs, unsigned long *nsecs)
{
	off_t pos;
	ssize_t r;

	assert(resultsfile >= 0);

	pos = groupid * (sizeof(*secs) + sizeof(*nsecs));
	if (lseek(resultsfile, pos, SEEK_SET) == -1) {
		err(1, "%s: lseek", RESULTSFILE);
	}
	r = read(resultsfile, secs, sizeof(*secs));
	if (r < 0) {
		err(1, "%s: read (seconds)", RESULTSFILE);
	}
	if ((size_t)r < sizeof(*secs)) {
		errx(1, "%s: read (seconds): Unexpected EOF", RESULTSFILE);
	}
	r = read(resultsfile, nsecs, sizeof(*nsecs));
	if (r < 0) {
		err(1, "%s: read (nsecs)", RESULTSFILE);
	}
	if ((size_t)r < sizeof(*nsecs)) {
		errx(1, "%s: read (nsecs): Unexpected EOF", RESULTSFILE);
	}
}
