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

/*
 * rmtest.c
 *
 * 	Tests file system synchronization by deleting an open file and
 * 	then attempting to read it.
 *
 * This should run correctly when the file system assignment is complete.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include <test161/test161.h>

// 23 Mar 2012 : GWA : BUFFER_COUNT must be even.

#define BUFFER_COUNT 128
#define BUFFER_SIZE 128

static int writebuf[BUFFER_SIZE];
static int readbuf[BUFFER_SIZE];

int
main(int argc, char **argv)
{

	// 23 Mar 2012 : GWA : Assume argument passing is *not* supported.

	(void) argc;
	(void) argv;
	int i, j;
	int fh, len;
	off_t pos, target;

	const char * filename = "fileonlytest.dat";

	// 23 Mar 2012 : GWA : Test that open works.

	tprintf("Opening %s\n", filename);

	fh = open(filename, O_RDWR|O_CREAT|O_TRUNC);
	if (fh < 0) {
		err(1, "create failed");
	}

	tprintf("Writing %d bytes.\n", BUFFER_SIZE * BUFFER_COUNT);

  // 23 Mar 2012 : GWA : Do the even-numbered writes. Test read() and
  // lseek(SEEK_END).

  for (i = 0; i < BUFFER_COUNT / 2; i++) {
		for (j = 0; j < BUFFER_SIZE; j++) {
			writebuf[j] = i * 2 * j;
		}
		len = write(fh, writebuf, sizeof(writebuf));
		if (len != sizeof(writebuf)) {
			err(1, "write failed");
		}

    // 23 Mar 2012 : GWA : Use lseek() to skip the odd guys.

    target = (i + 1) * 2 * sizeof(writebuf);
    pos = lseek(fh, sizeof(writebuf), SEEK_END);
    if (pos != target) {
      err(1, "(even) lseek failed: %llu != %llu", pos, target);
    }
  }

  target = 0;
  pos = lseek(fh, target, SEEK_SET);
  if (pos != target) {
    err(1, "(reset) lseek failed: %llu != %llu", pos, target);
  }

  // 23 Mar 2012 : GWA : Do the odd-numbered writes. Test write() and
  // lseek(SEEK_CUR).

  for (i = 0; i < BUFFER_COUNT / 2; i++) {

    // 23 Mar 2012 : GWA : Use lseek() to skip the even guys.

    target = ((i * 2) + 1) * sizeof(writebuf);
    pos = lseek(fh, sizeof(writebuf), SEEK_CUR);
    if (pos != target) {
      err(1, "(odd) lseek failed: %llu != %llu", pos, target);
    }

    for (j = 0; j < BUFFER_SIZE; j++) {
			writebuf[j] = ((i * 2) + 1) * j;
		}
		len = write(fh, writebuf, sizeof(writebuf));
		if (len != sizeof(writebuf)) {
			err(1, "write failed");
		}
  }

	// 23 Mar 2012 : GWA : Read the test data back and make sure what we wrote
	// ended up where we wrote it. Tests read() and lseek(SEEK_SET).

	tprintf("Verifying write.\n");

	for (i = BUFFER_COUNT - 1; i >= 0; i--) {
    target = i * sizeof(writebuf);
		pos = lseek(fh, target, SEEK_SET);
    if (pos != target) {
      err(1, "(verify) lseek failed: %llu != %llu", pos, target);
    }
		len = read(fh, readbuf, sizeof(readbuf));
		if (len != sizeof(readbuf)) {
			err(1, "read failed");
		}
		for (j = BUFFER_SIZE - 1; j >= 0; j--) {
			if (readbuf[j] != i * j) {
				err(1, "read mismatch: pos=%llu, readbuf[j]=%d, i*j=%d, i=%d, j=%d", pos, readbuf[j], i * j, i, j);
			}
		}
	}

	// 23 Mar 2012 : GWA : Close the file.

	tprintf("Closing %s\n", filename);
	close(fh);

	// 23 Mar 2012 : GWA : Make sure the file is actually closed.

	pos = lseek(fh, (off_t) 0, SEEK_SET);
	if (pos == 0) {
		err(1, "seek after close succeeded");
	}

	success(TEST161_SUCCESS, SECRET, "/testbin/fileonlytest");
	return 0;
}
