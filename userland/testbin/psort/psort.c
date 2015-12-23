/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009, 2014
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
 * psort - parallel sort.
 *
 * This is loosely based on some real parallel sort benchmarks, but
 * because of various limitations of OS/161 it is massively
 * inefficient. But that's ok; the goal is to stress the VM and buffer
 * cache.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifndef RANDOM_MAX
/* Note: this is correct for OS/161 but not for some Unix C libraries */
#define RANDOM_MAX RAND_MAX
#endif

#define PATH_KEYS    "sortkeys"
#define PATH_SORTED  "output"
#define PATH_TESTDIR "psortdir"
#define PATH_RANDOM  "rand:"

/*
 * Workload sizing.
 *
 * We fork numprocs processes. Each one works on WORKNUM integers at a
 * time, so the total VM load is WORKNUM * sizeof(int) * numprocs. For
 * the best stress testing, this should be substantially larger than
 * your available RAM.
 *
 * Meanwhile we generate and process numkeys integers, so the total
 * filesystem load is numkeys * sizeof(int). For the best stress
 * testing this should be substantially larger than your buffer cache.
 *
 * This test is supposed to adjust to arbitrary settings of WORKNUM,
 * numprocs, and numkeys. For a small test try these settings:
 *      WORKNUM         (16*1024)
 *      numprocs        4, or 2
 *      numkeys         10000
 *
 * This should fit in memory (memory footprint is 256K) and generate
 * only small files (~40K) which will work on the pre-largefiles SFS.
 *
 * The default settings are:
 *      WORKNUM         (96*1024)
 *      numprocs        4
 *      numkeys         128*1024
 *
 * so the memory footprint is 1.5M (comparable to the VM tests) and
 * the base file size is 512K. Note that because there are a lot of
 * temporary files, it pumps a good deal more than 512K of data. For
 * the record, the solution set takes about 7.5 minutes of virtual
 * time to run it, in either 1M or 2M of RAM and with 4 CPUs; this
 * currently corresponds to about 13-14 minutes of real time.
 *
 * Note that the parent psort process serves as a director and doesn't
 * itself compute; it has a workspace (because it's a static buffer)
 * but doesn't use it. A VM system that doesn't do zerofill
 * optimization will be a lot slower because it has to copy this space
 * for every batch of forks.
 *
 * Also note that you can set numprocs and numkeys on the command
 * line, but not WORKNUM.
 *
 * FUTURE: maybe make a build option to malloc the work space instead
 * of using a static buffer, which would allow choosing WORKNUM on the
 * command line too, at the cost of depending on malloc working.
 */

/* Set the workload size. */
#define WORKNUM      (96*1024)
static int numprocs = 4;
static int numkeys = 128*1024;

/* Per-process work buffer */
static int workspace[WORKNUM];

/* Random seed for generating the data */
static long randomseed = 15432753;

/* other state */
static off_t correctsize;
static unsigned long checksum;

#define NOBODY (-1)
static int me = NOBODY;

static const char *progname;

////////////////////////////////////////////////////////////

static
void
sortints(int *v, int num)
{
	int pivotval, pivotpoint, pivotcount;
	int frontpos, readpos, endpos, i, j;
	int tmp;

	if (num < 2) {
		return;
	}

	pivotpoint = num/2;
	pivotval = v[pivotpoint];
	pivotcount = 0;

	frontpos = 0;
	readpos = 0;
	endpos = num;
	while (readpos < endpos) {
		if (v[readpos] < pivotval) {
			v[frontpos++] = v[readpos++];
		}
		else if (v[readpos] == pivotval) {
			readpos++;
			pivotcount++;
		}
		else {
			tmp = v[--endpos];
			v[endpos] = v[readpos];
			v[readpos] = tmp;
		}
	}
	assert(readpos == endpos);
	assert(frontpos + pivotcount == readpos);

	for (i=frontpos; i<endpos; i++) {
		v[i] = pivotval;
	}

	for (i=endpos, j=num-1; i<j; i++,j--) {
		tmp = v[i];
		v[i] = v[j];
		v[j] = tmp;
	}

	sortints(v, frontpos);
	sortints(&v[endpos], num-endpos);
}

////////////////////////////////////////////////////////////

static
void
initprogname(const char *av0)
{
	if (av0) {
		progname = strrchr(av0, '/');
		if (progname) {
			progname++;
		}
		else {
			progname = av0;
		}
	}
	else {
		progname = "psort";
	}
}

static
void
vscomplain(char *buf, size_t len, const char *fmt, va_list ap, int err)
{
	size_t pos;

	if (me >= 0) {
		snprintf(buf, len, "%s: proc %d: ", progname, me);
	}
	else {
		snprintf(buf, len, "%s: ", progname);
	}
	pos = strlen(buf);

	vsnprintf(buf+pos, len-pos, fmt, ap);
	pos = strlen(buf);

	if (err >= 0) {
		snprintf(buf+pos, len-pos, ": %s\n", strerror(err));
	}
	else {
		snprintf(buf+pos, len-pos, "\n");
	}
}

static
void
complainx(const char *fmt, ...)
{
	char buf[256];
	va_list ap;
	ssize_t junk;

	va_start(ap, fmt);
	vscomplain(buf, sizeof(buf), fmt, ap, -1);
	va_end(ap);

	/* Write the message in one go so it's atomic */
	junk = write(STDERR_FILENO, buf, strlen(buf));

	/*
	 * This variable must be assigned and then ignored with some
	 * (broken) Linux C libraries. Ah, Linux...
	 */
	(void)junk;
}

static
void
complain(const char *fmt, ...)
{
	char buf[256];
	va_list ap;
	ssize_t junk;
	int err = errno;

	va_start(ap, fmt);
	vscomplain(buf, sizeof(buf), fmt, ap, err);
	va_end(ap);

	/* Write the message in one go so it's atomic */
	junk = write(STDERR_FILENO, buf, strlen(buf));

	/*
	 * This variable must be assigned and then ignored with some
	 * (broken) Linux C libraries. Ah, Linux...
	 */
	(void)junk;
}

////////////////////////////////////////////////////////////

static
int
doopen(const char *path, int flags, int mode)
{
	int fd;

	fd = open(path, flags, mode);
	if (fd<0) {
		complain("%s", path);
		exit(1);
	}
	return fd;
}

static
void
doclose(const char *path, int fd)
{
	if (close(fd)) {
		complain("%s: close", path);
		exit(1);
	}
}

static
void
docreate(const char *path)
{
	int fd;

	fd = doopen(path, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	doclose(path, fd);
}

static
void
doremove(const char *path)
{
	static int noremove;

	if (noremove) {
		return;
	}

	if (remove(path) < 0) {
		if (errno == ENOSYS) {
			/* Complain (and try) only once. */
			noremove = 1;
		}
		complain("%s: remove", path);
	}
}

static
off_t
getsize(const char *path)
{
	struct stat buf;
	int fd;
	static int no_stat, no_fstat;

	if (!no_stat) {
		if (stat(path, &buf) == 0) {
			return buf.st_size;
		}
		if (errno != ENOSYS) {
			complain("%s: stat", path);
			exit(1);
		}
		/* Avoid further "Unknown syscall 81" messages */
		no_stat = 1;
	}

	fd = doopen(path, O_RDONLY, 0);
	if (!no_fstat) {
		if (fstat(fd, &buf) == 0) {
			close(fd);
			return buf.st_size;
		}
		if (errno != ENOSYS) {
			complain("%s: stat", path);
			exit(1);
		}
		/* Avoid further "Unknown syscall 82" messages */
		no_fstat = 1;
	}

	/* otherwise, lseek */
	if (lseek(fd, 0, SEEK_END) >= 0) {
		buf.st_size = lseek(fd, 0, SEEK_CUR);
		if (buf.st_size >= 0) {
			return buf.st_size;
		}
	}
	complain("%s: getting file size with lseek", path);
	close(fd);
	exit(1);
}

static
size_t
doread(const char *path, int fd, void *buf, size_t len)
{
	int result;

	result = read(fd, buf, len);
	if (result < 0) {
		complain("%s: read", path);
		exit(1);
	}
	return (size_t) result;
}

static
void
doexactread(const char *path, int fd, void *buf, size_t len)
{
	size_t result;

	result = doread(path, fd, buf, len);
	if (result != len) {
		complainx("%s: read: short count", path);
		exit(1);
	}
}

static
void
dowrite(const char *path, int fd, const void *buf, size_t len)
{
	int result;

	result = write(fd, buf, len);
	if (result < 0) {
		complain("%s: write", path);
		exit(1);
	}
	if ((size_t) result != len) {
		complainx("%s: write: short count", path);
		exit(1);
	}
}

static
void
dolseek(const char *name, int fd, off_t offset, int whence)
{
	if (lseek(fd, offset, whence) < 0) {
		complain("%s: lseek", name);
		exit(1);
	}
}

#if 0 /* let's not require subdirs */
static
void
dochdir(const char *path)
{
	if (chdir(path) < 0) {
		complain("%s: chdir", path);
		exit(1);
	}
}

static
void
domkdir(const char *path, int mode)
{
	if (mkdir(path, mode) < 0) {
		complain("%s: mkdir", path);
		exit(1);
	}
}
#endif /* 0 */

static
pid_t
dofork(void)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		complain("fork");
		/* but don't exit */
	}

	return pid;
}

////////////////////////////////////////////////////////////

static
int
dowait(int guy, pid_t pid)
{
	int status, result;

	result = waitpid(pid, &status, 0);
	if (result < 0) {
		complain("waitpid");
		return -1;
	}
	if (WIFSIGNALED(status)) {
		complainx("proc %d: signal %d", guy, WTERMSIG(status));
		return -1;
	}
	assert(WIFEXITED(status));
	status = WEXITSTATUS(status);
	if (status) {
		complainx("proc %d: exit %d", guy, status);
		return -1;
	}
	return 0;
}

static
void
doforkall(const char *phasename, void (*func)(void))
{
	int i, bad = 0;
	pid_t pids[numprocs];

	for (i=0; i<numprocs; i++) {
		pids[i] = dofork();
		if (pids[i] < 0) {
			bad = 1;
		}
		else if (pids[i] == 0) {
			/* child */
			me = i;
			func();
			exit(0);
		}
	}

	for (i=0; i<numprocs; i++) {
		if (pids[i] > 0 && dowait(i, pids[i])) {
			bad = 1;
		}
	}

	if (bad) {
		complainx("%s failed.", phasename);
		exit(1);
	}
}

static
void
seekmyplace(const char *name, int fd)
{
	int keys_per, myfirst;
	off_t offset;

	keys_per = numkeys / numprocs;
	myfirst = me*keys_per;
	offset = myfirst * sizeof(int);

	dolseek(name, fd, offset, SEEK_SET);
}

static
int
getmykeys(void)
{
	int keys_per, myfirst, mykeys;

	keys_per = numkeys / numprocs;
	myfirst = me*keys_per;
	mykeys = (me < numprocs-1) ? keys_per : numkeys - myfirst;

	return mykeys;
}

////////////////////////////////////////////////////////////

static
unsigned long
checksum_file(const char *path)
{
	int fd;
	char buf[512];
	size_t count, i;
	unsigned long sum = 0;

	fd = doopen(path, O_RDONLY, 0);

	while ((count = doread(path, fd, buf, sizeof(buf))) > 0) {
		for (i=0; i<count; i++) {
			sum += (unsigned char) buf[i];
		}
	}

	doclose(path, fd);

	return sum;
}

////////////////////////////////////////////////////////////

static long *seeds;

static
void
genkeys_sub(void)
{
	int fd, i, mykeys, keys_done, keys_to_do, value;

	fd = doopen(PATH_KEYS, O_WRONLY, 0);

	mykeys = getmykeys();
	seekmyplace(PATH_KEYS, fd);

	srandom(seeds[me]);
	keys_done = 0;
	while (keys_done < mykeys) {
		keys_to_do = mykeys - keys_done;
		if (keys_to_do > WORKNUM) {
			keys_to_do = WORKNUM;
		}

		for (i=0; i<keys_to_do; i++) {
			value = random();

			// check bounds of value
			assert(value >= 0);
			assert(value <= RANDOM_MAX);

			// do not allow the value to be zero or RANDOM_MAX
			while (value == 0 || value == RANDOM_MAX) {
				value = random();
			}

			workspace[i] = value;
		}

		dowrite(PATH_KEYS, fd, workspace, keys_to_do*sizeof(int));
		keys_done += keys_to_do;
	}

	doclose(PATH_KEYS, fd);
}

static
void
genkeys(void)
{
	long seedspace[numprocs];
	int i;

	/* Create the file. */
	docreate(PATH_KEYS);

	/* Generate random seeds for each subprocess. */
	srandom(randomseed);
	for (i=0; i<numprocs; i++) {
		seedspace[i] = random();
	}

	/* Do it. */
	complainx("Generating %d integers using %d procs", numkeys, numprocs);
	seeds = seedspace;
	doforkall("Initialization", genkeys_sub);
	seeds = NULL;

	/* Cross-check the size of the output. */
	if (getsize(PATH_KEYS) != correctsize) {
		complainx("%s: file is wrong size", PATH_KEYS);
		exit(1);
	}

	/* Checksum the output. */
	complainx("Checksumming the data (using one proc)");
	checksum = checksum_file(PATH_KEYS);
	complainx("Checksum of unsorted keys: %ld", checksum);
}

////////////////////////////////////////////////////////////

static
const char *
binname(int a, int b)
{
	static char rv[32];
	snprintf(rv, sizeof(rv), "bin-%d-%d", a, b);
	return rv;
}

static
const char *
mergedname(int a)
{
	static char rv[32];
	snprintf(rv, sizeof(rv), "merged-%d", a);
	return rv;
}

static
void
bin(void)
{
	int infd, outfds[numprocs];
	const char *name;
	int i, mykeys, keys_done, keys_to_do;
	int key, pivot, binnum;

	infd = doopen(PATH_KEYS, O_RDONLY, 0);

	mykeys = getmykeys();
	seekmyplace(PATH_KEYS, infd);

	for (i=0; i<numprocs; i++) {
		name = binname(me, i);
		outfds[i] = doopen(name, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	}

	pivot = (RANDOM_MAX / numprocs);

	keys_done = 0;
	while (keys_done < mykeys) {
		keys_to_do = mykeys - keys_done;
		if (keys_to_do > WORKNUM) {
			keys_to_do = WORKNUM;
		}

		doexactread(PATH_KEYS, infd, workspace,
			    keys_to_do * sizeof(int));

		for (i=0; i<keys_to_do; i++) {
			key = workspace[i];

			binnum = key / pivot;
			if (key <= 0) {
				complainx("proc %d: garbage key %d", me, key);
				key = 0;
			}
			assert(binnum >= 0);
			assert(binnum < numprocs);
			dowrite("bin", outfds[binnum], &key, sizeof(key));
		}

		keys_done += keys_to_do;
	}
	doclose(PATH_KEYS, infd);

	for (i=0; i<numprocs; i++) {
		doclose(binname(me, i), outfds[i]);
	}
}

static
void
sortbins(void)
{
	const char *name;
	int i, fd;
	off_t binsize;

	for (i=0; i<numprocs; i++) {
		name = binname(me, i);
		binsize = getsize(name);
		if (binsize % sizeof(int) != 0) {
			complainx("%s: bin size %ld no good", name,
				  (long) binsize);
			exit(1);
		}
		if (binsize > (off_t) sizeof(workspace)) {
			complainx("proc %d: %s: bin too large", me, name);
			exit(1);
		}

		fd = doopen(name, O_RDWR, 0);
		doexactread(name, fd, workspace, binsize);

		sortints(workspace, binsize/sizeof(int));

		dolseek(name, fd, 0, SEEK_SET);
		dowrite(name, fd, workspace, binsize);
		doclose(name, fd);
	}
}

static
void
mergebins(void)
{
	int infds[numprocs], outfd;
	int values[numprocs], ready[numprocs];
	const char *name, *outname;
	int i, result;
	int numready, place, val, worknum;

	outname = mergedname(me);
	outfd = doopen(outname, O_WRONLY|O_CREAT|O_TRUNC, 0664);

	for (i=0; i<numprocs; i++) {
		name = binname(i, me);
		infds[i] = doopen(name, O_RDONLY, 0);
		values[i] = 0;
		ready[i] = 0;
	}

	worknum = 0;

	while (1) {
		numready = 0;
		for (i=0; i<numprocs; i++) {
			if (infds[i] < 0) {
				continue;
			}

			if (!ready[i]) {
				result = doread("bin", infds[i],
						&val, sizeof(int));
				if (result == 0) {
					doclose("bin", infds[i]);
					infds[i] = -1;
					continue;
				}
				if ((size_t) result != sizeof(int)) {
					complainx("%s: read: short count",
						  binname(i, me));
					exit(1);
				}
				values[i] = val;
				ready[i] = 1;
			}
			numready++;
		}
		if (numready == 0) {
			break;
		}

		/* find the smallest */
		place = -1;
		for (i=0; i<numprocs; i++) {
			if (!ready[i]) {
				continue;
			}
			if (place < 0 || values[i] < val) {
				val = values[i];
				place = i;
			}
		}
		assert(place >= 0);

		workspace[worknum++] = val;
		if (worknum >= WORKNUM) {
			assert(worknum == WORKNUM);
			dowrite(outname, outfd, workspace,
				worknum * sizeof(int));
			worknum = 0;
		}
		ready[place] = 0;
	}

	dowrite(outname, outfd, workspace, worknum * sizeof(int));
	doclose(outname, outfd);

	for (i=0; i<numprocs; i++) {
		assert(infds[i] < 0);
	}
}

static
void
assemble(void)
{
	off_t mypos;
	int i, fd;
	const char *args[3];

	mypos = 0;
	for (i=0; i<me; i++) {
		mypos += getsize(mergedname(i));
	}

	fd = doopen(PATH_SORTED, O_WRONLY, 0);
	dolseek(PATH_SORTED, fd, mypos, SEEK_SET);

	if (dup2(fd, STDOUT_FILENO) < 0) {
		complain("dup2");
		exit(1);
	}

	doclose(PATH_SORTED, fd);

	args[0] = "cat";
	args[1] = mergedname(me);
	args[2] = NULL;
	execv("/bin/cat", (char **) args);
	complain("/bin/cat: exec");
	exit(1);
}

static
void
checksize_bins(void)
{
	off_t totsize;
	int i, j;

	totsize = 0;
	for (i=0; i<numprocs; i++) {
		for (j=0; j<numprocs; j++) {
			totsize += getsize(binname(i, j));
		}
	}
	if (totsize != correctsize) {
		complain("Sum of bin sizes is wrong (%ld, should be %ld)",
			 (long) totsize, (long) correctsize);
		exit(1);
	}
}

static
void
checksize_merge(void)
{
	off_t totsize;
	int i;

	totsize = 0;
	for (i=0; i<numprocs; i++) {
		totsize += getsize(mergedname(i));
	}
	if (totsize != correctsize) {
		complain("Sum of merged sizes is wrong (%ld, should be %ld)",
			 (long) totsize, (long) correctsize);
		exit(1);
	}
}

static
void
sort(void)
{
	unsigned long sortedsum;
	int i, j;

	/* Step 1. Toss into bins. */
	complainx("Tossing into %d bins using %d procs",
		  numprocs*numprocs, numprocs);
	doforkall("Tossing", bin);
	checksize_bins();
	complainx("Done tossing into bins.");

	/* Step 2: Sort the bins. */
	complainx("Sorting %d bins using %d procs",
		  numprocs*numprocs, numprocs);
	doforkall("Sorting", sortbins);
	checksize_bins();
	complainx("Done sorting the bins.");

	/* Step 3: Merge corresponding bins. */
	complainx("Merging %d bins using %d procs",
		  numprocs*numprocs, numprocs);
	doforkall("Merging", mergebins);
	checksize_merge();
	complainx("Done merging the bins.");

	/* Step 3a: delete the bins */
	for (i=0; i<numprocs; i++) {
		for (j=0; j<numprocs; j++) {
			doremove(binname(i, j));
		}
	}

	/* Step 4: assemble output file */
	complainx("Assembling output file using %d procs", numprocs);
	docreate(PATH_SORTED);
	doforkall("Final assembly", assemble);
	if (getsize(PATH_SORTED) != correctsize) {
		complainx("%s: file is wrong size", PATH_SORTED);
		exit(1);
	}

	/* Step 4a: delete the merged bins */
	for (i=0; i<numprocs; i++) {
		doremove(mergedname(i));
	}

	/* Step 5: Checksum the result. */
	complainx("Checksumming the output (using one proc)");
	sortedsum = checksum_file(PATH_SORTED);
	complainx("Checksum of sorted keys: %ld", sortedsum);

	if (sortedsum != checksum) {
		complainx("Sums do not match");
		exit(1);
	}
}

////////////////////////////////////////////////////////////

static
const char *
validname(int a)
{
	static char rv[32];
	snprintf(rv, sizeof(rv), "valid-%d", a);
	return rv;
}

static
void
checksize_valid(void)
{
	off_t totvsize, correctvsize;
	int i;

	correctvsize = (off_t) numprocs*2*sizeof(int);

	totvsize = 0;
	for (i=0; i<numprocs; i++) {
		totvsize += getsize(validname(i));
	}
	if (totvsize != correctvsize) {
		complainx("Sum of validation sizes is wrong "
			  "(%ld, should be %ld)",
			  (long) totvsize, (long) correctvsize);
		exit(1);
	}
}

static
void
dovalidate(void)
{
	const char *name;
	int fd, i, mykeys, keys_done, keys_to_do;
	int key, smallest, largest;

	name = PATH_SORTED;
	fd = doopen(name, O_RDONLY, 0);

	mykeys = getmykeys();
	seekmyplace(name, fd);

	smallest = RANDOM_MAX;
	largest = 0;

	keys_done = 0;
	while (keys_done < mykeys) {
		keys_to_do = mykeys - keys_done;
		if (keys_to_do > WORKNUM) {
			keys_to_do = WORKNUM;
		}

		doexactread(name, fd, workspace, keys_to_do * sizeof(int));

		for (i=0; i<keys_to_do; i++) {
			key = workspace[i];

			if (key < 0) {
				complain("%s: found negative key", name);
				exit(1);
			}
			if (key == 0) {
				complain("%s: found zero key", name);
				exit(1);
			}
			if (key >= RANDOM_MAX) {
				complain("%s: found too-large key", name);
				exit(1);
			}

			if (key < smallest) {
				smallest = key;
			}
			if (key > largest) {
				largest = key;
			}
		}

		keys_done += keys_to_do;
	}
	doclose(name, fd);

	name = validname(me);
	fd = doopen(name, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	dowrite(name, fd, &smallest, sizeof(smallest));
	dowrite(name, fd, &largest, sizeof(largest));
	doclose(name, fd);
}

static
void
validate(void)
{
	int smallest, largest, prev_largest;
	int i, fd;
	const char *name;

	complainx("Validating the sorted data using %d procs", numprocs);
	doforkall("Validation", dovalidate);
	checksize_valid();

	prev_largest = 1;

	for (i=0; i<numprocs; i++) {
		name = validname(i);
		fd = doopen(name, O_RDONLY, 0);

		doexactread(name, fd, &smallest, sizeof(int));
		doexactread(name, fd, &largest, sizeof(int));

		if (smallest < 1) {
			complainx("Validation: block %d: bad SMALLEST", i);
			exit(1);
		}
		if (largest >= RANDOM_MAX) {
			complainx("Validation: block %d: bad LARGEST", i);
			exit(1);
		}
		if (smallest > largest) {
			complainx("Validation: block %d: SMALLEST > LARGEST",
				  i);
			exit(1);
		}

		if (smallest < prev_largest) {
			complain("Validation: block %d smallest key %d",
				 i, smallest);
			complain("Validation: previous block largest key %d",
				 prev_largest);
			complain("Validation failed");
			exit(1);
		}
	}


	for (i=0; i<numprocs; i++) {
		doremove(validname(i));
	}
}

////////////////////////////////////////////////////////////

static
void
setdir(void)
{
#if 0 /* let's not require subdirs */
	domkdir(PATH_TESTDIR, 0775);
	dochdir(PATH_TESTDIR);
#endif /* 0 */
}

static
void
unsetdir(void)
{
	doremove(PATH_KEYS);
	doremove(PATH_SORTED);
#if 0 /* let's not require subdirs */
	dochdir("..");

	if (rmdir(PATH_TESTDIR) < 0) {
		complain("%s: rmdir", PATH_TESTDIR);
		/* but don't exit */
	}
#endif /* 0 */
}

////////////////////////////////////////////////////////////

static
void
randomize(void)
{
	int fd;

	fd = doopen(PATH_RANDOM, O_RDONLY, 0);
	doexactread(PATH_RANDOM, fd, &randomseed, sizeof(randomseed));
	doclose(PATH_RANDOM, fd);
}

static
void
usage(void)
{
	complain("Usage: %s [-p procs] [-k keys] [-s seed] [-r]", progname);
	exit(1);
}

static
void
doargs(int argc, char *argv[])
{
	int i, ch, val, arg;

	for (i=1; i<argc; i++) {
		if (argv[i][0] != '-') {
			usage();
			return;
		}
		ch = argv[i][1];
		switch (ch) {
		    case 'p': arg = 1; break;
		    case 'k': arg = 1; break;
		    case 's': arg = 1; break;
		    case 'r': arg = 0; break;
		    default: usage(); return;
		}
		if (arg) {
			if (argv[i][2]) {
				val = atoi(argv[i]+2);
			}
			else {
				i++;
				if (!argv[i]) {
					complain("Option -%c requires an "
						 "argument", ch);
					exit(1);
				}
				val = atoi(argv[i]);
			}
			switch (ch) {
			    case 'p': numprocs = val; break;
			    case 'k': numkeys = val; break;
			    case 's': randomseed = val; break;
			    default: assert(0); break;
			}
		}
		else {
			switch (ch) {
			    case 'r': randomize(); break;
			    default: assert(0); break;
			}
		}
	}
}

int
main(int argc, char *argv[])
{
	initprogname(argc > 0 ? argv[0] : NULL);

	doargs(argc, argv);
	correctsize = (off_t) (numkeys*sizeof(int));

	setdir();

	genkeys();
	sort();
	validate();
	complainx("Succeeded.");

	unsetdir();

	return 0;
}
