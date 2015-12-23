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
 * fstest - filesystem test code
 *
 * Writes a file (in small chunks) and then reads it back again
 * (also in small chunks) and complains if what it reads back is
 * not the same.
 *
 * The length of SLOGAN is intentionally a prime number and
 * specifically *not* a power of two.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <synch.h>
#include <vfs.h>
#include <fs.h>
#include <vnode.h>
#include <test.h>

#define SLOGAN   "HODIE MIHI - CRAS TIBI\n"
#define FILENAME "fstest.tmp"
#define NCHUNKS  720
#define NTHREADS 12
#define NLONG    32
#define NCREATE  24

static struct semaphore *threadsem = NULL;

static
void
init_threadsem(void)
{
	if (threadsem==NULL) {
		threadsem = sem_create("fstestsem", 0);
		if (threadsem == NULL) {
			panic("fstest: sem_create failed\n");
		}
	}
}

/*
 * Vary each line of the test file in a way that's predictable but
 * unlikely to mask bugs in the filesystem.
 */
static
void
rotate(char *str, int amt)
{
	int i, ch;

	amt = (amt+2600)%26;
	KASSERT(amt>=0);

	for (i=0; str[i]; i++) {
		ch = str[i];
		if (ch>='A' && ch<='Z') {
			ch = ch - 'A';
			ch += amt;
			ch %= 26;
			ch = ch + 'A';
			KASSERT(ch>='A' && ch<='Z');
		}
		str[i] = ch;
	}
}

////////////////////////////////////////////////////////////

static
void
fstest_makename(char *buf, size_t buflen,
		const char *fs, const char *namesuffix)
{
	snprintf(buf, buflen, "%s:%s%s", fs, FILENAME, namesuffix);
	KASSERT(strlen(buf) < buflen);
}

#define MAKENAME() fstest_makename(name, sizeof(name), fs, namesuffix)

static
int
fstest_remove(const char *fs, const char *namesuffix)
{
	char name[32];
	char buf[32];
	int err;

	MAKENAME();

	strcpy(buf, name);
	err = vfs_remove(buf);
	if (err) {
		kprintf("Could not remove %s: %s\n", name, strerror(err));
		return -1;
	}

	return 0;
}

static
int
fstest_write(const char *fs, const char *namesuffix,
	     int stridesize, int stridepos)
{
	struct vnode *vn;
	int err;
	int i;
	size_t shouldbytes=0;
	size_t bytes=0;
	off_t pos=0;
	char name[32];
	char buf[32];
	struct iovec iov;
	struct uio ku;
	int flags;

	KASSERT(sizeof(buf) > strlen(SLOGAN));

	MAKENAME();

	flags = O_WRONLY|O_CREAT;
	if (stridesize == 1) {
		flags |= O_TRUNC;
	}

	/* vfs_open destroys the string it's passed */
	strcpy(buf, name);
	err = vfs_open(buf, flags, 0664, &vn);
	if (err) {
		kprintf("Could not open %s for write: %s\n",
			name, strerror(err));
		return -1;
	}

	for (i=0; i<NCHUNKS; i++) {
		if (i % stridesize != stridepos) {
			pos += strlen(SLOGAN);
			continue;
		}
		strcpy(buf, SLOGAN);
		rotate(buf, i);
		uio_kinit(&iov, &ku, buf, strlen(SLOGAN), pos, UIO_WRITE);
		err = VOP_WRITE(vn, &ku);
		if (err) {
			kprintf("%s: Write error: %s\n", name, strerror(err));
			vfs_close(vn);
			vfs_remove(name);
			return -1;
		}

		if (ku.uio_resid > 0) {
			kprintf("%s: Short write: %lu bytes left over\n",
				name, (unsigned long) ku.uio_resid);
			vfs_close(vn);
			vfs_remove(name);
			return -1;
		}

		bytes += (ku.uio_offset - pos);
		shouldbytes += strlen(SLOGAN);
		pos = ku.uio_offset;
	}

	vfs_close(vn);

	if (bytes != shouldbytes) {
		kprintf("%s: %lu bytes written, should have been %lu!\n",
			name, (unsigned long) bytes,
			(unsigned long) (NCHUNKS*strlen(SLOGAN)));
		vfs_remove(name);
		return -1;
	}
	kprintf("%s: %lu bytes written\n", name, (unsigned long) bytes);

	return 0;
}

static
int
fstest_read(const char *fs, const char *namesuffix)
{
	struct vnode *vn;
	int err;
	int i;
	size_t bytes=0;
	char name[32];
	char buf[32];
	struct iovec iov;
	struct uio ku;

	MAKENAME();

	/* vfs_open destroys the string it's passed */
	strcpy(buf, name);
	err = vfs_open(buf, O_RDONLY, 0664, &vn);
	if (err) {
		kprintf("Could not open test file for read: %s\n",
			strerror(err));
		return -1;
	}

	for (i=0; i<NCHUNKS; i++) {
		uio_kinit(&iov, &ku, buf, strlen(SLOGAN), bytes, UIO_READ);
		err = VOP_READ(vn, &ku);
		if (err) {
			kprintf("%s: Read error: %s\n", name, strerror(err));
			vfs_close(vn);
			return -1;
		}

		if (ku.uio_resid > 0) {
			kprintf("%s: Short read: %lu bytes left over\n", name,
				(unsigned long) ku.uio_resid);
			vfs_close(vn);
			return -1;
		}
		buf[strlen(SLOGAN)] = 0;
		rotate(buf, -i);
		if (strcmp(buf, SLOGAN)) {
			kprintf("%s: Test failed: line %d mismatched: %s\n",
				name, i+1, buf);
			vfs_close(vn);
			return -1;
		}

		bytes = ku.uio_offset;
	}

	vfs_close(vn);

	if (bytes != NCHUNKS*strlen(SLOGAN)) {
		kprintf("%s: %lu bytes read, should have been %lu!\n",
			name, (unsigned long) bytes,
			(unsigned long) (NCHUNKS*strlen(SLOGAN)));
		return -1;
	}
	kprintf("%s: %lu bytes read\n", name, (unsigned long) bytes);
	return 0;
}

////////////////////////////////////////////////////////////

static
void
dofstest(const char *filesys)
{
	kprintf("*** Starting filesystem test on %s:\n", filesys);

	if (fstest_write(filesys, "", 1, 0)) {
		kprintf("*** Test failed\n");
		return;
	}

	if (fstest_read(filesys, "")) {
		kprintf("*** Test failed\n");
		return;
	}

	if (fstest_remove(filesys, "")) {
		kprintf("*** Test failed\n");
		return;
	}

	kprintf("*** Filesystem test done\n");
}

////////////////////////////////////////////////////////////

static
void
readstress_thread(void *fs, unsigned long num)
{
	const char *filesys = fs;
	if (fstest_read(filesys, "")) {
		kprintf("*** Thread %lu: failed\n", num);
	}
	V(threadsem);
}

static
void
doreadstress(const char *filesys)
{
	int i, err;

	init_threadsem();

	kprintf("*** Starting fs read stress test on %s:\n", filesys);

	if (fstest_write(filesys, "", 1, 0)) {
		kprintf("*** Test failed\n");
		return;
	}

	for (i=0; i<NTHREADS; i++) {
		err = thread_fork("readstress", NULL,
				  readstress_thread, (char *)filesys, i);
		if (err) {
			panic("readstress: thread_fork failed: %s\n",
			      strerror(err));
		}
	}

	for (i=0; i<NTHREADS; i++) {
		P(threadsem);
	}

	if (fstest_remove(filesys, "")) {
		kprintf("*** Test failed\n");
		return;
	}

	kprintf("*** fs read stress test done\n");
}

////////////////////////////////////////////////////////////

static
void
writestress_thread(void *fs, unsigned long num)
{
	const char *filesys = fs;
	char numstr[8];
	snprintf(numstr, sizeof(numstr), "%lu", num);

	if (fstest_write(filesys, numstr, 1, 0)) {
		kprintf("*** Thread %lu: failed\n", num);
		V(threadsem);
		return;
	}

	if (fstest_read(filesys, numstr)) {
		kprintf("*** Thread %lu: failed\n", num);
		V(threadsem);
		return;
	}

	if (fstest_remove(filesys, numstr)) {
		kprintf("*** Thread %lu: failed\n", num);
	}

	kprintf("*** Thread %lu: done\n", num);

	V(threadsem);
}

static
void
dowritestress(const char *filesys)
{
	int i, err;

	init_threadsem();

	kprintf("*** Starting fs write stress test on %s:\n", filesys);

	for (i=0; i<NTHREADS; i++) {
		err = thread_fork("writestress", NULL,
				  writestress_thread, (char *)filesys, i);
		if (err) {
			panic("thread_fork failed %s\n", strerror(err));
		}
	}

	for (i=0; i<NTHREADS; i++) {
		P(threadsem);
	}

	kprintf("*** fs write stress test done\n");
}

////////////////////////////////////////////////////////////

static
void
writestress2_thread(void *fs, unsigned long num)
{
	const char *filesys = fs;

	if (fstest_write(filesys, "", NTHREADS, num)) {
		kprintf("*** Thread %lu: failed\n", num);
		V(threadsem);
		return;
	}

	V(threadsem);
}

static
void
dowritestress2(const char *filesys)
{
	int i, err;
	char name[32];
	struct vnode *vn;

	init_threadsem();

	kprintf("*** Starting fs write stress test 2 on %s:\n", filesys);

	/* Create and truncate test file */
	fstest_makename(name, sizeof(name), filesys, "");
	err = vfs_open(name, O_WRONLY|O_CREAT|O_TRUNC, 0664, &vn);
	if (err) {
		kprintf("Could not create test file: %s\n", strerror(err));
		kprintf("*** Test failed\n");
		return;
	}
	vfs_close(vn);

	for (i=0; i<NTHREADS; i++) {
		err = thread_fork("writestress2", NULL,
				  writestress2_thread, (char *)filesys, i);
		if (err) {
			panic("writestress2: thread_fork failed: %s\n",
			      strerror(err));
		}
	}

	for (i=0; i<NTHREADS; i++) {
		P(threadsem);
	}

	if (fstest_read(filesys, "")) {
		kprintf("*** Test failed\n");
		return;
	}

	if (fstest_remove(filesys, "")) {
		kprintf("*** Test failed\n");
	}


	kprintf("*** fs write stress test 2 done\n");
}

////////////////////////////////////////////////////////////

static
void
longstress_thread(void *fs, unsigned long num)
{
	const char *filesys = fs;
	int i;
	char numstr[16];

	for (i=0; i<NLONG; i++) {

		snprintf(numstr, sizeof(numstr), "%lu-%d", num, i);

		if (fstest_write(filesys, numstr, 1, 0)) {
			kprintf("*** Thread %lu: file %d: failed\n", num, i);
			V(threadsem);
			return;
		}

		if (fstest_read(filesys, numstr)) {
			kprintf("*** Thread %lu: file %d: failed\n", num, i);
			V(threadsem);
			return;
		}

		if (fstest_remove(filesys, numstr)) {
			kprintf("*** Thread %lu: file %d: failed\n", num, i);
			V(threadsem);
			return;
		}

	}

	V(threadsem);
}

static
void
dolongstress(const char *filesys)
{
	int i, err;

	init_threadsem();

	kprintf("*** Starting fs long stress test on %s:\n", filesys);

	for (i=0; i<NTHREADS; i++) {
		err = thread_fork("longstress", NULL,
				  longstress_thread, (char *)filesys, i);
		if (err) {
			panic("longstress: thread_fork failed %s\n",
			      strerror(err));
		}
	}

	for (i=0; i<NTHREADS; i++) {
		P(threadsem);
	}

	kprintf("*** fs long stress test done\n");
}

////////////////////////////////////////////////////////////

static
void
createstress_thread(void *fs, unsigned long num)
{
	const char *filesys = fs;
	int i, err;
	char namesuffix[16];
	char name[32];
	char buf[32];
	struct vnode *vn;
	struct iovec iov;
	struct uio ku;
	size_t bytes;
	unsigned numwritten = 0, numread = 0, numremoved = 0;

	for (i=0; i<NCREATE; i++) {
		snprintf(namesuffix, sizeof(namesuffix), "%lu-%d", num, i);
		MAKENAME();

		/* vfs_open destroys the string it's passed */
		strcpy(buf, name);
		err = vfs_open(buf, O_WRONLY|O_CREAT|O_TRUNC, 0664, &vn);
		if (err) {
			kprintf("Could not open %s for write: %s\n",
				name, strerror(err));
			continue;
		}

		strcpy(buf, SLOGAN);
		rotate(buf, i);

		uio_kinit(&iov, &ku, buf, strlen(SLOGAN), 0, UIO_WRITE);
		err = VOP_WRITE(vn, &ku);
		vfs_close(vn);
		if (err) {
			kprintf("%s: Write error: %s\n", name, strerror(err));
			continue;
		}
		if (ku.uio_resid > 0) {
			kprintf("%s: Short write: %lu bytes left over\n",
				name, (unsigned long) ku.uio_resid);
			continue;
		}

		bytes = ku.uio_offset;
		if (bytes != strlen(SLOGAN)) {
			kprintf("%s: %lu bytes written, expected %lu!\n",
				name, (unsigned long) bytes,
				(unsigned long) strlen(SLOGAN));
			continue;
		}
		numwritten++;
	}
	kprintf("Thread %lu: %u files written\n", num, numwritten);

	for (i=0; i<NCREATE; i++) {
		snprintf(namesuffix, sizeof(namesuffix), "%lu-%d", num, i);
		MAKENAME();

		/* vfs_open destroys the string it's passed */
		strcpy(buf, name);
		err = vfs_open(buf, O_RDONLY, 0664, &vn);
		if (err) {
			kprintf("Could not open %s for read: %s\n",
				name, strerror(err));
			continue;
		}

		uio_kinit(&iov, &ku, buf, strlen(SLOGAN), 0, UIO_READ);
		err = VOP_READ(vn, &ku);
		vfs_close(vn);
		if (err) {
			kprintf("%s: Read error: %s\n", name, strerror(err));
			continue;
		}
		if (ku.uio_resid > 0) {
			kprintf("%s: Short read: %lu bytes left over\n",
				name, (unsigned long) ku.uio_resid);
			continue;
		}

		buf[strlen(SLOGAN)] = 0;
		rotate(buf, -i);

		if (strcmp(buf, SLOGAN)) {
			kprintf("%s: Test failed: file mismatched: %s\n",
				name, buf);
			continue;
		}

		bytes = ku.uio_offset;
		if (bytes != strlen(SLOGAN)) {
			kprintf("%s: %lu bytes read, expected %lu!\n",
				name, (unsigned long) bytes,
				(unsigned long) strlen(SLOGAN));
			continue;
		}

		numread++;
	}
	kprintf("Thread %lu: %u files read\n", num, numread);

	for (i=0; i<NCREATE; i++) {
		snprintf(namesuffix, sizeof(namesuffix), "%lu-%d", num, i);
		if (fstest_remove(filesys, namesuffix)) {
			continue;
		}
		numremoved++;
	}
	kprintf("Thread %lu: %u files removed\n", num, numremoved);

	V(threadsem);
}

static
void
docreatestress(const char *filesys)
{
	int i, err;

	init_threadsem();

	kprintf("*** Starting fs create stress test on %s:\n", filesys);

	for (i=0; i<NTHREADS; i++) {
		err = thread_fork("createstress", NULL,
				  createstress_thread, (char *)filesys, i);
		if (err) {
			panic("createstress: thread_fork failed %s\n",
			      strerror(err));
		}
	}

	for (i=0; i<NTHREADS; i++) {
		P(threadsem);
	}

	kprintf("*** fs create stress test done\n");
}

////////////////////////////////////////////////////////////

static
int
checkfilesystem(int nargs, char **args)
{
	char *device;

	if (nargs != 2) {
		kprintf("Usage: fs[12345] filesystem:\n");
		return EINVAL;
	}

	device = args[1];

	/* Allow (but do not require) colon after device name */
	if (device[strlen(device)-1]==':') {
		device[strlen(device)-1] = 0;
	}

	return 0;
}

#define DEFTEST(testname)                         \
  int                                             \
  testname(int nargs, char **args)                \
  {                                               \
	int result;                               \
	result = checkfilesystem(nargs, args);    \
	if (result) {                             \
		return result;                    \
	}                                         \
	do##testname(args[1]);                    \
	return 0;                                 \
  }

DEFTEST(fstest);
DEFTEST(readstress);
DEFTEST(writestress);
DEFTEST(writestress2);
DEFTEST(longstress);
DEFTEST(createstress);

////////////////////////////////////////////////////////////

int
printfile(int nargs, char **args)
{
	struct vnode *rv, *wv;
	struct iovec iov;
	struct uio ku;
	off_t rpos=0, wpos=0;
	char buf[128];
	char outfile[16];
	int result;
	int done=0;

	if (nargs != 2) {
		kprintf("Usage: pf filename\n");
		return EINVAL;
	}

	/* vfs_open destroys the string it's passed; make a copy */
	strcpy(outfile, "con:");

	result = vfs_open(args[1], O_RDONLY, 0664, &rv);
	if (result) {
		kprintf("printfile: %s\n", strerror(result));
		return result;
	}

	result = vfs_open(outfile, O_WRONLY, 0664, &wv);
	if (result) {
		kprintf("printfile: output: %s\n", strerror(result));
		vfs_close(rv);
		return result;
	}

	while (!done) {
		uio_kinit(&iov, &ku, buf, sizeof(buf), rpos, UIO_READ);
		result = VOP_READ(rv, &ku);
		if (result) {
			kprintf("Read error: %s\n", strerror(result));
			break;
		}
		rpos = ku.uio_offset;

		if (ku.uio_resid > 0) {
			done = 1;
		}

		uio_kinit(&iov, &ku, buf, sizeof(buf)-ku.uio_resid, wpos,
			  UIO_WRITE);
		result = VOP_WRITE(wv, &ku);
		if (result) {
			kprintf("Write error: %s\n", strerror(result));
			break;
		}
		wpos = ku.uio_offset;

		if (ku.uio_resid > 0) {
			kprintf("Warning: short write\n");
		}
	}

	vfs_close(wv);
	vfs_close(rv);

	return 0;
}
