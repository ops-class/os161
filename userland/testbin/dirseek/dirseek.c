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
 * dirseek.c
 *
 *      Tests seeking on directories (both legally and illegally).
 *
 *      Makes a test subdirectory in the current directory.
 *
 *      Intended for the file system assignment. Should run (on SFS)
 *      when that assignment is complete.
 *
 *      Note: checks a few things that are not _strictly_ guaranteed
 *      by the official semantics of getdirentry() but that are more
 *      or less necessary in a sane implementation, like that the
 *      current seek position returned after seeking is the same
 *      position that was requested. If you believe your
 *      implementation is legal and the the test is rejecting it
 *      gratuitously, please contact the course staff.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#define TESTDIR "seektestdir"

static struct {
	const char *name;
	int make_it;
	off_t pos;
} testfiles[] = {
	{ ".", 		0, -1 },
	{ "..",		0, -1 },
	{ "ridcully",	1, -1 },
	{ "weatherwax",	1, -1 },
	{ "ogg",	1, -1 },
	{ "vorbis",	1, -1 },
	{ "verence",	1, -1 },
	{ "magrat",	1, -1 },
	{ "agnes",	1, -1 },
	{ "rincewind",	1, -1 },
	{ "angua",	1, -1 },
	{ "cherry",	1, -1 },
	{ "dorfl",	1, -1 },
	{ "nobby",	1, -1 },
	{ "carrot",	1, -1 },
	{ "vimes",	1, -1 },
	{ "detritus",	1, -1 },
	{ "twoflower",	1, -1 },
	{ "teatime",	1, -1 },
	{ "qu",		1, -1 },
	{ NULL, 0, 0 }
};

/************************************************************/
/* Test code                                                */
/************************************************************/

static int dirfd;

static
int
findentry(const char *name)
{
	int i;

	for (i=0; testfiles[i].name; i++) {
		if (!strcmp(testfiles[i].name, name)) {
			return i;
		}
	}
	return -1;
}

static
void
openit(void)
{
	dirfd = open(".", O_RDONLY);
	if (dirfd < 0) {
		err(1, ".: open");
	}
}

static
void
closeit(void)
{
	if (close(dirfd)<0) {
		err(1, ".: close");
	}
	dirfd = -1;
}

static
void
readit(void)
{
	char buf[4096];
	off_t pos;
	int len;
	int n, i, ix;

	for (i=0; testfiles[i].name; i++) {
		testfiles[i].pos = -1;
	}

	pos = lseek(dirfd, 0, SEEK_CUR);
	if (pos < 0) {
		err(1, ".: lseek(0, SEEK_CUR)");
	}
	n = 0;

	while ((len = getdirentry(dirfd, buf, sizeof(buf)-1)) > 0) {

		if ((unsigned)len >= sizeof(buf)-1) {
			errx(1, ".: entry %d: getdirentry returned "
			     "invalid length %d", n, len);
		}
		buf[len] = 0;
		ix = findentry(buf);
		if (ix < 0) {
			errx(1, ".: entry %d: getdirentry returned "
			     "unexpected name %s", n, buf);
		}

		if (testfiles[ix].pos >= 0) {
			errx(1, ".: entry %d: getdirentry returned "
			     "%s a second time", n, buf);
		}

		testfiles[ix].pos = pos;

		pos = lseek(dirfd, 0, SEEK_CUR);
		if (pos < 0) {
			err(1, ".: lseek(0, SEEK_CUR)");
		}
		n++;
	}
	if (len<0) {
		err(1, ".: entry %d: getdirentry", n);
	}

	for (i=0; testfiles[i].name; i++) {
		if (testfiles[i].pos < 0) {
			errx(1, ".: getdirentry failed to return %s",
			     testfiles[i].name);
		}
	}
	if (i!=n) {
		/*
		 * If all of the other checks have passed, this should not
		 * be able to fail. But... just in case I forgot something
		 * or there's a bug...
		 */

		errx(1, ".: getdirentry returned %d names, not %d (huh...?)",
		     n, i);
	}
}

static
void
firstread(void)
{
	off_t pos;

	pos = lseek(dirfd, 0, SEEK_CUR);
	if (pos < 0) {
		err(1, ".: lseek(0, SEEK_CUR)");
	}
	if (pos != 0) {
		errx(1, ".: File position after open not 0");
	}

	tprintf("Scanning directory...\n");

	readit();
}

static
void
doreadat0(void)
{
	off_t pos;

	tprintf("Rewinding directory and reading it again...\n");

	pos = lseek(dirfd, 0, SEEK_SET);
	if (pos < 0) {
		err(1, ".: lseek(0, SEEK_SET)");
	}
	if (pos != 0) {
		errx(1, ".: lseek(0, SEEK_SET) returned %ld", (long) pos);
	}

	readit();
}

static
void
readone(const char *shouldbe)
{
	char buf[4096];
	int len;

	len = getdirentry(dirfd, buf, sizeof(buf)-1);
	if (len < 0) {
		err(1, ".: getdirentry");
	}
	if ((unsigned)len >= sizeof(buf)-1) {
		errx(1, ".: getdirentry returned invalid length %d", len);
	}
	buf[len] = 0;

	if (strcmp(buf, shouldbe)) {
		errx(1, ".: getdirentry returned %s (expected %s)",
		     buf, shouldbe);
	}
}

static
void
doreadone(int which)
{
	off_t pos;
	pos = lseek(dirfd, testfiles[which].pos, SEEK_SET);
	if (pos<0) {
		err(1, ".: lseek(%ld, SEEK_SET)", (long) testfiles[which].pos);
	}
	if (pos != testfiles[which].pos) {
		errx(1, ".: lseek(%ld, SEEK_SET) returned %ld",
		     (long) testfiles[which].pos, (long) pos);
	}

	readone(testfiles[which].name);
}

static
void
readallonebyone(void)
{
	int i;

	tprintf("Trying to read each entry again...\n");
	for (i=0; testfiles[i].name; i++) {
		doreadone(i);
	}
}

static
void
readallrandomly(void)
{
	int n, i, x;

	tprintf("Trying to read a bunch of entries randomly...\n");

	for (i=0; testfiles[i].name; i++);
	n = i;

	srandom(39584);
	for (i=0; i<512; i++) {
		x = (int)(random()%n);
		doreadone(x);
	}
}

static
void
readateof(void)
{
	char buf[4096];
	int len;

	len = getdirentry(dirfd, buf, sizeof(buf)-1);
	if (len < 0) {
		err(1, ".: at EOF: getdirentry");
	}
	if (len==0) {
		return;
	}
	if ((unsigned)len >= sizeof(buf)-1) {
		errx(1, ".: at EOF: getdirentry returned "
		     "invalid length %d", len);
	}
	buf[len] = 0;
	errx(1, ".: at EOF: got unexpected name %s", buf);
}

static
void
doreadateof(void)
{
	off_t pos;
	int i;

	tprintf("Trying to read after going to EOF...\n");

	pos = lseek(dirfd, 0, SEEK_END);
	if (pos<0) {
		err(1, ".: lseek(0, SEEK_END)");
	}

	for (i=0; testfiles[i].name; i++) {
		if (pos <= testfiles[i].pos) {
			errx(1, ".: EOF position %ld below position %ld of %s",
			     pos, testfiles[i].pos, testfiles[i].name);
		}
	}

	readateof();
}

static
void
inval_read(void)
{
	char buf[4096];
	int len;

	len = getdirentry(dirfd, buf, sizeof(buf)-1);

	/* Any result is ok, as long as the system doesn't crash */
	(void)len;
}

static
void
dobadreads(void)
{
	off_t pos, pos2, eof;
	int valid, i, k=0;

	tprintf("Trying some possibly invalid reads...\n");

	eof = lseek(dirfd, 0, SEEK_END);
	if (eof < 0) {
		err(1, ".: lseek(0, SEEK_END)");
	}

	for (pos=0; pos < eof; pos++) {
		valid = 0;
		for (i=0; testfiles[i].name; i++) {
			if (pos==testfiles[i].pos) {
				valid = 1;
			}
		}
		if (valid) {
			/* don't try offsets that are known to be valid */
			continue;
		}

		pos2 = lseek(dirfd, pos, SEEK_SET);
		if (pos2 < 0) {
			/* this is ok */
		}
		else {
			inval_read();
			k++;
		}
	}

	if (k>0) {
		tprintf("Survived %d invalid reads...\n", k);
	}
	else {
		tprintf("Couldn't find any invalid offsets to try...\n");
	}

	tprintf("Trying to read beyond EOF...\n");
	pos2 = lseek(dirfd, eof + 1000, SEEK_SET);
	if (pos2 < 0) {
		/* this is ok */
	}
	else {
		inval_read();
	}
}

static
void
dotest(void)
{
	tprintf("Opening directory...\n");
	openit();

	tprintf("Running tests...\n");

	/* read the whole directory */
	firstread();

	/* make sure eof behaves right */
	readateof();

	/* read all the filenames again by seeking */
	readallonebyone();

	/* try reading at eof */
	doreadateof();

	/* read a bunch of the filenames over and over again */
	readallrandomly();

	/* rewind and read the whole thing again, to make sure that works */
	doreadat0();

	/* do invalid reads */
	dobadreads();

	/* rewind again to make sure the invalid attempts didn't break it */
	doreadat0();

	tprintf("Closing directory...\n");
	closeit();
}

/************************************************************/
/* Setup code                                               */
/************************************************************/

static
void
mkfile(const char *name)
{
	int fd, i, r;
	static const char message[] = "The turtle moves!\n";
	char buf[32*sizeof(message)+1];

	buf[0]=0;
	for (i=0; i<32; i++) {
		strcat(buf, message);
	}

	/* Use O_EXCL, because we know the file shouldn't already be there */
	fd = open(name, O_WRONLY|O_CREAT|O_EXCL, 0664);
	if (fd<0) {
		err(1, "%s: create", name);
	}

	r = write(fd, buf, strlen(buf));
	if (r<0) {
		err(1, "%s: write", name);
	}
	if ((unsigned)r != strlen(buf)) {
		errx(1, "%s: short write (%d bytes)", name, r);
	}

	if (close(fd)<0) {
		err(1, "%s: close", name);
	}
}

static
void
setup(void)
{
	int i;

	tprintf("Making directory %s...\n", TESTDIR);

	/* Create a directory */
	if (mkdir(TESTDIR, 0775)<0) {
		err(1, "%s: mkdir", TESTDIR);
	}

	/* Switch to it */
	if (chdir(TESTDIR)<0) {
		err(1, "%s: chdir", TESTDIR);
	}

	tprintf("Making some files...\n");

	/* Populate it */
	for (i=0; testfiles[i].name; i++) {
		if (testfiles[i].make_it) {
			mkfile(testfiles[i].name);
		}
		testfiles[i].pos = -1;
	}
}

static
void
cleanup(void)
{
	int i;

	tprintf("Cleaning up...\n");

	/* Remove the files */
	for (i=0; testfiles[i].name; i++) {
		if (testfiles[i].make_it) {
			if (remove(testfiles[i].name)<0) {
				err(1, "%s: remove", testfiles[i].name);
			}
		}
	}

	/* Leave the dir */
	if (chdir("..")<0) {
		err(1, "..: chdir");
	}

	/* Remove the dir */
	if (rmdir(TESTDIR)<0) {
		err(1, "%s: rmdir", TESTDIR);
	}
}


int
main(void)
{
	setup();

	/* Do the whole thing twice */
	dotest();
	dotest();

	cleanup();
	return 0;
}
