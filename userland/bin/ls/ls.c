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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <err.h>

/*
 * ls - list files.
 * Usage: ls [-adlRs] [files]
 *    -a   Show files whose names begin with a dot.
 *    -d   Don't list contents of directories specified on the command line.
 *    -l   Long format listing.
 *    -R   Recurse into subdirectories found.
 *    -s   (with -l) Show block counts.
 */

/* Flags for which options we're using. */
static int aopt=0;
static int dopt=0;
static int lopt=0;
static int Ropt=0;
static int sopt=0;

/* Process an option character. */
static
void
option(int ch)
{
	switch (ch) {
		case 'a': aopt=1; break;
		case 'd': dopt=1; break;
		case 'l': lopt=1; break;
		case 'R': Ropt=1; break;
		case 's': sopt=1; break;
	        default:
			errx(1, "Unknown option -%c", ch);
	}
}

/*
 * Utility function to find the non-directory part of a pathname.
 */
static
const char *
basename(const char *path)
{
	const char *s;

	s = strrchr(path, '/');
	if (s) {
		return s+1;
	}
	return path;
}

/*
 * Utility function to check if a name refers to a directory.
 */
static
int
isdir(const char *path)
{
	struct stat buf;
	int fd;

	/* Assume stat() may not be implemented; use fstat */
	fd = open(path, O_RDONLY);
	if (fd<0) {
		err(1, "%s", path);
	}
	if (fstat(fd, &buf)<0) {
		err(1, "%s: fstat", path);
	}
	close(fd);

	return S_ISDIR(buf.st_mode);
}

/*
 * When listing one of several subdirectories, show the name of the
 * directory.
 */
static
void
printheader(const char *file)
{
	/* No blank line before the first header */
	static int first=1;
	if (first) {
		first = 0;
	}
	else {
		printf("\n");
	}
	printf("%s:\n", file);
}

/*
 * Show a single file.
 * We don't do the neat multicolumn listing that Unix ls does.
 */
static
void
print(const char *path)
{
	struct stat statbuf;
	const char *file;
	int typech;

	if (lopt || sopt) {
		int fd;

		fd = open(path, O_RDONLY);
		if (fd<0) {
			err(1, "%s", path);
		}
		if (fstat(fd, &statbuf)<0) {
			err(1, "%s: fstat", path);
		}
		close(fd);
	}

	file = basename(path);

	if (sopt) {
		printf("%3d ", statbuf.st_blocks);
	}

	if (lopt) {
		if (S_ISREG(statbuf.st_mode)) {
			typech = '-';
		}
		else if (S_ISDIR(statbuf.st_mode)) {
			typech = 'd';
		}
		else if (S_ISLNK(statbuf.st_mode)) {
			typech = 'l';
		}
		else if (S_ISCHR(statbuf.st_mode)) {
			typech = 'c';
		}
		else if (S_ISBLK(statbuf.st_mode)) {
			typech = 'b';
		}
		else {
			typech = '?';
		}

		printf("%crwx------ %2d root  %-8llu",
		       typech,
		       statbuf.st_nlink,
		       statbuf.st_size);
	}
	printf("%s\n", file);
}

/*
 * List a directory.
 */
static
void
listdir(const char *path, int showheader)
{
	int fd;
	char buf[1024];
	char newpath[1024];
	ssize_t len;

	if (showheader) {
		printheader(path);
	}

	/*
	 * Open it.
	 */
	fd = open(path, O_RDONLY);
	if (fd<0) {
		err(1, "%s", path);
	}

	/*
	 * List the directory.
	 */
	while ((len = getdirentry(fd, buf, sizeof(buf)-1)) > 0) {
		buf[len] = 0;

		/* Assemble the full name of the new item */
		snprintf(newpath, sizeof(newpath), "%s/%s", path, buf);

		if (aopt || buf[0]!='.') {
			/* Print it */
			print(newpath);
		}
	}
	if (len<0) {
		err(1, "%s: getdirentry", path);
	}

	/* Done */
	close(fd);
}

static
void
recursedir(const char *path)
{
	int fd;
	char buf[1024];
	char newpath[1024];
	int len;

	/*
	 * Open it.
	 */
	fd = open(path, O_RDONLY);
	if (fd<0) {
		err(1, "%s", path);
	}

	/*
	 * List the directory.
	 */
	while ((len = getdirentry(fd, buf, sizeof(buf)-1)) > 0) {
		buf[len] = 0;

		/* Assemble the full name of the new item */
		snprintf(newpath, sizeof(newpath), "%s/%s", path, buf);

		if (!aopt && buf[0]=='.') {
			/* skip this one */
			continue;
		}

		if (!strcmp(buf, ".") || !strcmp(buf, "..")) {
			/* always skip these */
			continue;
		}

		if (!isdir(newpath)) {
			continue;
		}

		listdir(newpath, 1 /*showheader*/);
		if (Ropt) {
			recursedir(newpath);
		}
	}
	if (len<0) {
		err(1, "%s", path);
	}

	close(fd);
}

static
void
listitem(const char *path, int showheader)
{
	if (!dopt && isdir(path)) {
		listdir(path, showheader || Ropt);
		if (Ropt) {
			recursedir(path);
		}
	}
	else {
		print(path);
	}
}

int
main(int argc, char *argv[])
{
	int i,j, items=0;

	/*
	 * Go through the arguments and count how many non-option args.
	 */
	for (i=1; i<argc; i++) {
		if (argv[i][0]!='-') {
			items++;
		}
	}

	/*
	 * Now go through the options for real, processing them.
	 */
	for (i=1; i<argc; i++) {
		if (argv[i][0]=='-') {
			/*
			 * This word is an option.
			 * Process all the option characters in it.
			 */
			for (j=1; argv[i][j]; j++) {
				option(argv[i][j]);
			}
		}
		else {
			/*
			 * This word isn't an option; list it.
			 */
			listitem(argv[i], items>1);
		}
	}

	/*
	 * If no filenames were specified to list, list the current
	 * directory.
	 */
	if (items==0) {
		listitem(".", 0);
	}

	return 0;
}
