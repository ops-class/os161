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
 * rmdirtest.c
 *
 *      Tests file system synchronization and directory implementation by
 *      removing the current directory under itself and then trying to do
 *      things. It's ok for most of those things to fail, but the system
 *      shouldn't crash.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <err.h>


static const char testdir[] = "testdir";
static char startpoint[PATH_MAX - sizeof(testdir)];

/*
 * Create the test directory, and change into it, remembering
 * where we came from.
 */

static
void
startup(void)
{
	if (getcwd(startpoint, sizeof(startpoint))==NULL) {
		err(1, "getcwd (not in test dir)");
	}

	if (mkdir(testdir, 0775) < 0) {
		err(1, "%s: mkdir", testdir);
	}

	if (chdir(testdir) < 0) {
		err(1, "%s: chdir", testdir);
	}
}

/*
 * Remove the test directory.
 *
 * Note that even though it's the current directory, we can't do it
 * with rmdir(".") - what that would try to do is remove the "." entry
 * from the current directory, which is justifiably prohibited.
 */

static
void
killdir(void)
{
	char tmp[PATH_MAX];

	snprintf(tmp, sizeof(tmp), "%s/%s", startpoint, testdir);
	if (rmdir(tmp)<0) {
		err(1, "%s: rmdir", tmp);
	}
}

/*
 * Leave the test directory and go back to where we came from, so we
 * can try again.
 */

static
void
finish(void)
{
	if (chdir(startpoint)<0) {
		err(1, "%s: chdir", startpoint);
	}
}

/*************************************************************/

/*
 * Basic test - just try removing the directory without doing anything
 * evil.
 */
static
void
test1(void)
{
	printf("Making %s\n", testdir);
	startup();

	printf("Removing %s while in it\n", testdir);
	killdir();

	printf("Leaving the test directory\n");
	finish();
}

/*
 * Now do it while we also have the directory open.
 */

static
void
test2(void)
{
	int fd;

	printf("Now trying with the directory open...\n");
	startup();
	fd = open(".", O_RDONLY);
	if (fd<0) {
		err(1, ".: open");
	}
	killdir();
	finish();

	/* close *after* leaving, just for excitement */
	if (close(fd)<0) {
		err(1, "removed %s: close", testdir);
	}
}

/*
 * Now see if . and .. work after rmdir.
 */

static
void
test3(void)
{
	char buf[PATH_MAX];
	int fd;

	printf("Checking if . exists after rmdir\n");
	startup();
	killdir();

	fd = open(".", O_RDONLY);
	if (fd<0) {
		switch (errno) {
		    case EINVAL:
		    case EIO:
		    case ENOENT:
			break;
		    default:
			err(1, ".");
			break;
		}
	}
	else {
		close(fd);
	}

	fd = open("..", O_RDONLY);
	if (fd<0) {
		switch (errno) {
		    case EINVAL:
		    case EIO:
		    case ENOENT:
			break;
		    default:
			err(1, "..");
			break;
		}
	}
	else {
		warnx("..: openable after rmdir - might be bad");
		close(fd);
	}

	snprintf(buf, sizeof(buf), "../%s", testdir);
	fd = open(buf, O_RDONLY);
	if (fd<0) {
		switch (errno) {
		    case EINVAL:
		    case EIO:
		    case ENOENT:
			break;
		    default:
			err(1, "%s", buf);
			break;
		}
	}
	else {
		errx(1, "%s: works after rmdir", buf);
	}

	finish();
}

/*
 * Now try to create files.
 */

static
void
test4(void)
{
	char buf[4096];
	int fd;

	printf("Checking if creating files works after rmdir...\n");
	startup();
	killdir();

	fd = open("newfile", O_WRONLY|O_CREAT|O_TRUNC, 0664);
	if (fd<0) {
		switch (errno) {
		    case EINVAL:
		    case EIO:
		    case ENOENT:
			break;
		    default:
			err(1, "%s", buf);
			break;
		}
	}
	else {
		warnx("newfile: creating files after rmdir works");
		warnx("(this is only ok if the space gets reclaimed)");

		/*
		 * Waste a bunch of space so we'll be able to tell
		 */
		memset(buf, 'J', sizeof(buf));
		write(fd, buf, sizeof(buf));
		write(fd, buf, sizeof(buf));
		write(fd, buf, sizeof(buf));
		write(fd, buf, sizeof(buf));
		close(fd);
	}

	finish();
}

/*
 * Now try to create directories.
 */

static
void
test5(void)
{
	printf("Checking if creating subdirs works after rmdir...\n");
	startup();
	killdir();

	if (mkdir("newdir", 0775)<0) {
		switch (errno) {
		    case EINVAL:
		    case EIO:
		    case ENOENT:
			break;
		    default:
			err(1, "mkdir in removed dir");
			break;
		}
	}
	else {
		warnx("newfile: creating directories after rmdir works");
		warnx("(this is only ok if the space gets reclaimed)");

		/*
		 * Waste a bunch of space so we'll be able to tell
		 */
		mkdir("newdir/t0", 0775);
		mkdir("newdir/t1", 0775);
		mkdir("newdir/t2", 0775);
		mkdir("newdir/t3", 0775);
		mkdir("newdir/t4", 0775);
		mkdir("newdir/t5", 0775);
	}

	finish();
}

/*
 * Now try listing the directory.
 */
static
void
test6(void)
{
	char buf[PATH_MAX];
	int fd, len;

	printf("Now trying to list the directory...\n");
	startup();
	fd = open(".", O_RDONLY);
	if (fd<0) {
		err(1, ".: open");
	}
	killdir();

	while ((len = getdirentry(fd, buf, sizeof(buf)-1))>0) {
		if ((unsigned)len >= sizeof(buf)-1) {
			errx(1, ".: getdirentry: returned invalid length");
		}
		buf[len] = 0;
		if (!strcmp(buf, ".") || !strcmp(buf, "..")) {
			/* these are allowed to appear */
			continue;
		}
		errx(1, ".: getdirentry: returned unexpected name %s", buf);
	}
	if (len==0) {
		/* EOF - ok */
	}
	else { /* len < 0 */
		switch (errno) {
		    case EINVAL:
		    case EIO:
			break;
		    default:
			err(1, ".: getdirentry");
			break;
		}
	}

	finish();

	/* close *after* leaving, just for excitement */
	if (close(fd)<0) {
		err(1, "removed %s: close", testdir);
	}
}

/*
 * Try getcwd.
 */
static
void
test7(void)
{
	char buf[PATH_MAX];

	startup();
	killdir();
	if (getcwd(buf, sizeof(buf))==NULL) {
		switch (errno) {
		    case EINVAL:
		    case EIO:
		    case ENOENT:
			break;
		    default:
			err(1, "getcwd after removing %s", testdir);
			break;
		}
	}
	else {
		errx(1, "getcwd after removing %s: succeeded (got %s)",
		     testdir, buf);
	}

	finish();
}

/**************************************************************/

int
main(void)
{
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	test7();

	printf("Whew... survived.\n");
	return 0;
}
