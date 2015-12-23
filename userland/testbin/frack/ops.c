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

#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <err.h>

#include "pool.h"
#include "data.h"
#include "do.h"
#include "check.h"
#include "ops.h"
#include "main.h"

struct file {
	unsigned name;
	unsigned testcode;
	unsigned seq;
	int handle;
};

struct dir {
	unsigned name;
	int handle;
};

static int checkmode;

void
setcheckmode(int mode)
{
	checkmode = mode;
	if (checkmode) {
		check_setup();
	}
}

////////////////////////////////////////////////////////////
// open directories

#define MAXDIRS 32
DECLPOOL(dir, MAXDIRS);

struct dir *
op_opendir(unsigned name)
{
	struct dir *ret;

	ret = POOLALLOC(dir);
	ret->name = name;
	if (checkmode) {
		ret->handle = -1;
	}
	else {
		ret->handle = do_opendir(name);
	}
	return ret;
}

void
op_closedir(struct dir *d)
{
	if (checkmode) {
		/* nothing */
		(void)d;
	}
	else {
		do_closedir(d->handle, d->name);
	}
	POOLFREE(dir, d);
}

////////////////////////////////////////////////////////////
// files

#define MAXFILES 32
DECLPOOL(file, MAXFILES);

struct file *
op_open(unsigned testcode, unsigned name, unsigned openflags)
{
	struct file *ret;
	int dotrunc;

	if (openflags == O_TRUNC) {
		openflags = 0;
		dotrunc = 1;
	}
	else {
		dotrunc = 0;
	}

	assert(openflags == 0 || openflags == (O_CREAT|O_EXCL));

	ret = POOLALLOC(file);
	ret->name = name;
	ret->testcode = testcode;
	ret->seq = 0;
	if (checkmode) {
		if (openflags) {
			ret->handle = check_createfile(name);
		}
		else {
			ret->handle = check_openfile(name);
		}
	}
	else {
		if (openflags) {
			assert(dotrunc == 0);
			ret->handle = do_createfile(name);
		}
		else {
			/*
			 * XXX: as of 2013 OS/161 doesn't provide a
			 * truncate call - neither truncate() nor
			 * ftruncate()! You can only O_TRUNC. Oops...
			 */
			ret->handle = do_openfile(name, dotrunc);
			dotrunc = 0;
		}
	}
	if (dotrunc) {
		op_truncate(ret, 0);
	}
	return ret;
}

void
op_close(struct file *f)
{
	if (checkmode) {
		check_closefile(f->handle, f->name);
	}
	else {
		do_closefile(f->handle, f->name);
	}
	POOLFREE(file, f);
}

void
op_write(struct file *f, off_t pos, off_t len)
{
	off_t amount;

	while (len > 0) {
		amount = len;
		if (amount > DATA_MAXSIZE) {
			amount = DATA_MAXSIZE;
		}

		if (checkmode) {
			check_write(f->handle, f->name, f->testcode, f->seq,
				    pos, amount);
		}
		else {
			do_write(f->handle, f->name, f->testcode, f->seq,
				 pos, amount);
		}
		f->seq++;
		pos += amount;
		len -= amount;
	}
}

void
op_truncate(struct file *f, off_t len)
{
	if (checkmode) {
		check_truncate(f->handle, f->name, len);
	}
	else {
		do_truncate(f->handle, f->name, len);
	}
}

////////////////////////////////////////////////////////////
// dirops

void
op_mkdir(unsigned name)
{
	if (checkmode) {
		check_mkdir(name);
	}
	else {
		do_mkdir(name);
	}
}

void
op_rmdir(unsigned name)
{
	if (checkmode) {
		check_rmdir(name);
	}
	else {
		do_rmdir(name);
	}
}

void
op_unlink(unsigned name)
{
	if (checkmode) {
		check_unlink(name);
	}
	else {
		do_unlink(name);
	}
}

void
op_link(unsigned from, unsigned to)
{
	if (checkmode) {
		check_link(from, to);
	}
	else {
		do_link(from, to);
	}
}

void
op_rename(unsigned from, unsigned to)
{
	if (checkmode) {
		check_rename(from, to);
	}
	else {
		do_rename(from, to);
	}
}

void
op_renamexd(unsigned fromdir, unsigned from, unsigned todir, unsigned to)
{
	if (checkmode) {
		check_renamexd(fromdir, from, todir, to);
	}
	else {
		do_renamexd(fromdir, from, todir, to);
	}
}

void
op_chdir(unsigned name)
{
	if (checkmode) {
		check_chdir(name);
	}
	else {
		do_chdir(name);
	}
}

void
op_chdirup(void)
{
	if (checkmode) {
		check_chdirup();
	}
	else {
		do_chdirup();
	}
}

////////////////////////////////////////////////////////////
// other

void
op_sync(void)
{
	if (checkmode) {
		check_sync();
	}
	else {
		do_sync();
	}
}

void
complete(void)
{
	if (checkmode) {
		checkfs();
	}
}
