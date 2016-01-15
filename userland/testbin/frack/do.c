/*
 * Copyright (c) 2013
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "data.h"
#include "name.h"
#include "do.h"

int
do_opendir(unsigned name)
{
	const char *namestr;
	int fd;

	namestr = name_get(name);
	fd = open(namestr, O_RDONLY);
	if (fd < 0) {
		err(1, "%s: opendir", namestr);
	}
	return fd;
}

void
do_closedir(int fd, unsigned name)
{
	if (close(fd)) {
		warn("%s: closedir", name_get(name));
	}
}

int
do_createfile(unsigned name)
{
	const char *namestr;
	int fd;

	namestr = name_get(name);
	fd = open(namestr, O_WRONLY|O_CREAT|O_EXCL, 0664);
	if (fd < 0) {
		err(1, "%s: create", namestr);
	}
	tprintf("create %s\n", namestr);
	return fd;
}

int
do_openfile(unsigned name, int dotrunc)
{
	const char *namestr;
	int fd;

	namestr = name_get(name);
	fd = open(namestr, O_WRONLY | (dotrunc ? O_TRUNC : 0), 0664);
	if (fd < 0) {
		err(1, "%s: open", namestr);
	}
	return fd;
}

void
do_closefile(int fd, unsigned name)
{
	if (close(fd)) {
		warn("%s: close", name_get(name));
	}
}

void
do_write(int fd, unsigned name, unsigned code, unsigned seq,
	 off_t pos, off_t len)
{
	off_t done = 0;
	ssize_t ret;
	char *buf;
	const char *namestr;

	namestr = name_get(name);
	buf = data_map(code, seq, len);
	if (lseek(fd, pos, SEEK_SET) == -1) {
		err(1, "%s: lseek to %lld", name_get(name), pos);
	}

	while (done < len) {
		ret = write(fd, buf + done, len - done);
		if (ret == -1) {
			err(1, "%s: write %lld at %lld", name_get(name),
			    len, pos);
		}
		done += ret;
	}

	tprintf("write %s: %lld at %lld\n", namestr, len, pos);
}

void
do_truncate(int fd, unsigned name, off_t len)
{
	const char *namestr;

	namestr = name_get(name);
	if (ftruncate(fd, len) == -1) {
		err(1, "%s: truncate to %lld", namestr, len);
	}
	tprintf("truncate %s: to %lld\n", namestr, len);
}

void
do_mkdir(unsigned name)
{
	const char *namestr;

	namestr = name_get(name);
	if (mkdir(namestr, 0775) == -1) {
		err(1, "%s: mkdir", namestr);
	}
	tprintf("mkdir %s\n", namestr);
}

void
do_rmdir(unsigned name)
{
	const char *namestr;

	namestr = name_get(name);
	if (rmdir(namestr) == -1) {
		err(1, "%s: rmdir", namestr);
	}
	tprintf("rmdir %s\n", namestr);
}

void
do_unlink(unsigned name)
{
	const char *namestr;

	namestr = name_get(name);
	if (remove(namestr) == -1) {
		err(1, "%s: remove", namestr);
	}
	tprintf("remove %s\n", namestr);
}

void
do_link(unsigned from, unsigned to)
{
	const char *fromstr, *tostr;

	fromstr = name_get(from);
	tostr = name_get(to);
	if (link(fromstr, tostr) == -1) {
		err(1, "link %s to %s", fromstr, tostr);
	}
	tprintf("link %s %s\n", fromstr, tostr);
}

void
do_rename(unsigned from, unsigned to)
{
	const char *fromstr, *tostr;

	fromstr = name_get(from);
	tostr = name_get(to);
	if (rename(fromstr, tostr) == -1) {
		err(1, "rename %s to %s", fromstr, tostr);
	}
	tprintf("rename %s %s\n", fromstr, tostr);
}

void
do_renamexd(unsigned fromdir, unsigned from, unsigned todir, unsigned to)
{
	char frombuf[64];
	char tobuf[64];

	strcpy(frombuf, name_get(fromdir));
	strcat(frombuf, "/");
	strcat(frombuf, name_get(from));

	strcpy(tobuf, name_get(todir));
	strcat(tobuf, "/");
	strcat(tobuf, name_get(to));

	if (rename(frombuf, tobuf) == -1) {
		err(1, "rename %s to %s", frombuf, tobuf);
	}
	tprintf("rename %s %s\n", frombuf, tobuf);
}

void
do_chdir(unsigned name)
{
	const char *namestr;

	namestr = name_get(name);
	if (chdir(namestr) == -1) {
		err(1, "chdir: %s", namestr);
	}
	tprintf("chdir %s\n", namestr);
}

void
do_chdirup(void)
{
	if (chdir("..") == -1) {
		err(1, "chdir: ..");
	}
	tprintf("chdir ..\n");
}

void
do_sync(void)
{
	if (sync()) {
		warn("sync");
	}
	tprintf("sync\n");
	tprintf("----------------------------------------\n");
}
