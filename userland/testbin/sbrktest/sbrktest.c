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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <test161/test161.h>

#define _PATH_RANDOM   "random:"

/*
 * Caution: OS/161 doesn't provide any way to get this properly from
 * the kernel. The page size is 4K on almost all hardware... but not
 * all. If porting to certain weird machines this will need attention.
 */
#define PAGE_SIZE 4096

////////////////////////////////////////////////////////////
// support code

static
int
geti(void)
{
	int val=0;
	int ch, digits=0;

	while (1) {
		ch = getchar();
		if (ch=='\n' || ch=='\r') {
			putchar('\n');
			break;
		}
		else if ((ch=='\b' || ch==127) && digits>0) {
			tprintf("\b \b");
			val = val/10;
			digits--;
		}
		else if (ch>='0' && ch<='9') {
			putchar(ch);
			val = val*10 + (ch-'0');
			digits++;
		}
		else {
			putchar('\a');
		}
	}

	if (digits==0) {
		return -1;
	}
	return val;
}

static
unsigned long
getseed(void)
{
	int fd, len;
	unsigned long seed;

	fd = open(_PATH_RANDOM, O_RDONLY);
	if (fd < 0) {
		err(1, "%s", _PATH_RANDOM);
	}
	len = read(fd, &seed, sizeof(seed));
	if (len < 0) {
		err(1, "%s", _PATH_RANDOM);
	}
	else if (len < (int)sizeof(seed)) {
		errx(1, "%s: Short read", _PATH_RANDOM);
	}
	close(fd);

	return seed;
}

static
pid_t
dofork(void)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		err(1, "fork");
	}
	return pid;
}

static
void
dowait(pid_t pid)
{
	int status;
	int result;

	result = waitpid(pid, &status, 0);
	if (result == -1) {
		err(1, "waitpid");
	}
	if (WIFSIGNALED(status)) {
		errx(1, "child: Signal %d", WTERMSIG(status));
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		errx(1, "child: Exit %d", WEXITSTATUS(status));
	}
}

static
void
say(const char *msg)
{
	/* Use one write so it's atomic (tprintf usually won't be) */
	write(STDOUT_FILENO, msg, strlen(msg));
}

////////////////////////////////////////////////////////////
// memory checking

/*
 * Fill a page of memory with a test pattern.
 */
static
void
markpage(volatile void *baseptr, unsigned pageoffset)
{
	volatile char *pageptr;
	size_t n, i;
	volatile unsigned long *pl;
	unsigned long val;

	pageptr = baseptr;
	pageptr += (size_t)PAGE_SIZE * pageoffset;

	pl = (volatile unsigned long *)pageptr;
	n = PAGE_SIZE / sizeof(unsigned long);

	for (i=0; i<n; i++) {
		val = ((unsigned long)i ^ (unsigned long)pageoffset);
		pl[i] = val;
	}
}

/*
 * Check a page marked with markblock()
 */
static
int
checkpage(volatile void *baseptr, unsigned pageoffset, bool neednl)
{
	volatile char *pageptr;
	size_t n, i;
	volatile unsigned long *pl;
	unsigned long val;

	pageptr = baseptr;
	pageptr += (size_t)PAGE_SIZE * pageoffset;

	pl = (volatile unsigned long *)pageptr;
	n = PAGE_SIZE / sizeof(unsigned long);

	for (i=0; i<n; i++) {
		val = ((unsigned long)i ^ (unsigned long)pageoffset);
		if (pl[i] != val) {
			if (neednl) {
				tprintf("\n");
			}
			tprintf("FAILED: data mismatch at offset %lu of page "
			       "at 0x%lx: %lu vs. %lu\n",
			       (unsigned long) (i*sizeof(unsigned long)),
			       (unsigned long)(uintptr_t)pl,
			       pl[i], val);
			return -1;
		}
	}

	return 0;
}

/*
 * Light version; touches just the first word of a page.
 */
static
void
markpagelight(volatile void *baseptr, unsigned pageoffset)
{
	volatile char *pageptr;
	volatile unsigned long *pl;

	pageptr = baseptr;
	pageptr += (size_t)PAGE_SIZE * pageoffset;

	pl = (volatile unsigned long *)pageptr;
	pl[0] = pageoffset;
}

/*
 * Light version; checks just the first word of a page.
 */
static
int
checkpagelight(volatile void *baseptr, unsigned pageoffset, bool neednl)
{
	volatile char *pageptr;
	volatile unsigned long *pl;

	pageptr = baseptr;
	pageptr += (size_t)PAGE_SIZE * pageoffset;

	pl = (volatile unsigned long *)pageptr;
	if (pl[0] != pageoffset) {
		if (neednl) {
			tprintf("\n");
		}
		tprintf("FAILED: data mismatch at offset 0 of page "
		       "at 0x%lx: %lu vs. %u\n",
		       (unsigned long)(uintptr_t)pl,
		       pl[0], pageoffset);
		return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////
// error wrapper

static
void *
dosbrk(ssize_t size)
{
	void *p;

	p = sbrk(size);
	if (p == (void *)-1) {
		err(1, "FAILED: sbrk");
	}
	if (p == NULL) {
		errx(1, "FAILED: sbrk returned NULL, which is illegal");
	}
	return p;
}

////////////////////////////////////////////////////////////
// fork a child that segfaults

typedef void (*segfault_fn)(void);

static
void
expect_segfault(segfault_fn func)
{
	int status;
	int result;
	pid_t pid = dofork();

	if (pid == 0) {
		func();	// This exits
	} else {
		result = waitpid(pid, &status, 0);
		if (result == -1) {
			err(1, "waitpid");
		}
		else if (WIFSIGNALED(status)) {
			if (WTERMSIG(status) != 11) {
				errx(1, "child: Signal %d", WTERMSIG(status));
			}
		}
		else  {
			errx(1, "child exited, expected segfault");
		}
	}
}

////////////////////////////////////////////////////////////
// align the heap

static
void
setup(void)
{
	void *op;
	uintptr_t opx;
	size_t amount;
	int error;

	op = dosbrk(0);
	opx = (uintptr_t)op;

	if (opx % PAGE_SIZE) {
		amount = PAGE_SIZE - (opx % PAGE_SIZE);
		if (sbrk(amount) == (void *)-1) {
			error = errno;
			warnx("Initial heap was not page aligned");
			warnx("...and trying to align it gave: %s",
			      strerror(error));
		}
	}

	op = dosbrk(0);
	opx = (uintptr_t)op;

	if (opx % PAGE_SIZE) {
		warnx("Initial heap was not page aligned");
		errx(1, "...and trying to align it didn't take.");
	}
}

////////////////////////////////////////////////////////////
// simple allocation

/*
 * Allocate one page, check that it holds data, and leak it.
 */
static
void
test1(void)
{
	void *p;

	tprintf("Allocating a page...\n");
	p = dosbrk(PAGE_SIZE);
	markpage(p, 0);
	if (checkpage(p, 0, false)) {
		errx(1, "FAILED: data corrupt");
	}
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocate one page, check that it holds data, and free it.
 */
static
void
test2(void)
{
	void *op, *p, *q;

	op = dosbrk(0);

	tprintf("Allocating a page...\n");
	p = dosbrk(PAGE_SIZE);
	if (p != op) {
		errx(1, "FAILED: sbrk grow didn't return the old break "
		    "(got %p, expected %p", p, op);
	}
	markpage(p, 0);
	if (checkpage(p, 0, false)) {
		errx(1, "FAILED: data corrupt");
	}

	p = dosbrk(0);

	tprintf("Freeing the page...\n");
	q = dosbrk(-PAGE_SIZE);
	if (q != p) {
		errx(1, "FAILED: sbrk shrink didn't return the old break "
		     "(got %p, expected %p", q, p);
	}
	q = dosbrk(0);
	if (q != op) {
		errx(1, "FAILED: sbrk shrink didn't restore the heap "
		     "(got %p, expected %p", q, op);
	}
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocate six pages, check that they hold data and that the
 * pages don't get mixed up, and free them.
 */
static
void
test3(void)
{
	const unsigned num = 6;

	void *op, *p, *q;
	unsigned i;
	bool bad;

	op = dosbrk(0);

	tprintf("Allocating %u pages...\n", num);
	p = dosbrk(PAGE_SIZE * num);
	if (p != op) {
		errx(1, "FAILED: sbrk grow didn't return the old break "
		     "(got %p, expected %p", p, op);
	}

	bad = false;
	for (i=0; i<num; i++) {
		markpage(p, i);
		if (checkpage(p, i, false)) {
			warnx("FAILED: data corrupt on page %u", i);
			bad = true;
		}
	}
	if (bad) {
		exit(1);
	}

	p = dosbrk(0);

	tprintf("Freeing the pages...\n");
	q = dosbrk(-PAGE_SIZE * num);
	if (q != p) {
		errx(1, "FAILED: sbrk shrink didn't return the old break "
		     "(got %p, expected %p", q, p);
	}
	q = dosbrk(0);
	if (q != op) {
		errx(1, "FAILED: sbrk shrink didn't restore the heap "
		     "(got %p, expected %p", q, op);
	}
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocate six pages, check that they hold data and that the pages
 * don't get mixed up, and free them one at a time, repeating the
 * check after each free.
 */
static
void
test4(void)
{
	const unsigned num = 6;

	void *op, *p, *q;
	unsigned i, j;
	bool bad;

	op = dosbrk(0);

	tprintf("Allocating %u pages...\n", num);
	p = dosbrk(PAGE_SIZE * num);
	if (p != op) {
		errx(1, "FAILED: sbrk grow didn't return the old break "
		     "(got %p, expected %p", p, op);
	}

	bad = false;
	for (i=0; i<num; i++) {
		markpage(p, i);
		if (checkpage(p, i, false)) {
			warnx("FAILED: data corrupt on page %u", i);
			bad = true;
		}
	}
	if (bad) {
		exit(1);
	}

	tprintf("Freeing the pages one at a time...\n");
	for (i=num; i-- > 0; ) {
		(void)dosbrk(-PAGE_SIZE);
		for (j=0; j<i; j++) {
			if (checkpage(p, j, false)) {
				warnx("FAILED: data corrupt on page %u "
				      "after freeing %u pages", j, i);
				bad = true;
			}
		}
	}
	if (bad) {
		exit(1);
	}

	q = dosbrk(0);
	if (q != op) {
		errx(1, "FAILED: sbrk shrink didn't restore the heap "
		     "(got %p, expected %p", q, op);
	}
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

////////////////////////////////////////////////////////////
// crashing off the end

/*
 * Checks that the page past end of the heap as we got it is not
 * valid. (Crashes when successful.)
 */
static
void
test5_helper(void)
{
	void *p;

	p = dosbrk(0);
	tprintf("This should produce fatal signal 11 (SIGSEGV).\n");
	((long *)p)[10] = 0;
	errx(1, "FAILED: I didn't crash");
}

static
void
test5(void)
{
	expect_segfault(test5_helper);
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocates a page and checks that the next page past it is not
 * valid. (Crashes when successful.)
 */
static
void
test6_helper(void)
{
	void *p;

	(void)dosbrk(PAGE_SIZE);
	p = dosbrk(0);
	tprintf("This should produce fatal signal 11 (SIGSEGV).\n");
	((long *)p)[10] = 0;
	errx(1, "FAILED: I didn't crash");
}

static
void
test6(void)
{
	expect_segfault(test6_helper);
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocates and frees a page and checks that the page freed is no
 * longer valid. (Crashes when successful.)
 */
static
void
test7_helper(void)
{
	void *p;

	(void)dosbrk(PAGE_SIZE);
	(void)dosbrk(-PAGE_SIZE);
	p = dosbrk(0);
	tprintf("This should produce fatal signal 11 (SIGSEGV).\n");
	((long *)p)[10] = 0;
	errx(1, "FAILED: I didn't crash");
}

static
void
test7(void)
{
	expect_segfault(test7_helper);
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocates some pages, frees half of them, and checks that the page
 * past the new end of the heap is no longer valid. (Crashes when
 * successful.)
 */
static
void
test8_helper(void)
{
	void *p;

	(void)dosbrk(PAGE_SIZE * 12);
	(void)dosbrk(-PAGE_SIZE * 6);
	p = dosbrk(0);
	tprintf("This should produce fatal signal 11 (SIGSEGV).\n");
	((long *)p)[10] = 0;
	errx(1, "FAILED: I didn't crash");
}

static
void
test8(void)
{
	expect_segfault(test8_helper);
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

////////////////////////////////////////////////////////////
// heap size

/*
 * Allocate all memory at once.
 *
 * This works by trying successively smaller sizes until one succeeds.
 * Note that if you allow arbitrary swap overcommit this test will run
 * the system out of swap. If this kills the system that's probably ok
 * (check with your course staff, but handing OOM is difficult and
 * probably not worth the time it would take you)... but ideally no
 * one process is allowed to get big enough to do this.
 *
 * The recommended behavior is to set the process's maximum heap size
 * to the amount of available RAM + swap that can be used at once.
 * Because the test uses powers of two, this doesn't have to be very
 * precise (e.g. counting the size of the non-heap parts of the
 * process probably doesn't matter unless you're very unlucky) and
 * isn't very difficult.
 *
 * Another option is to disallow swap overcommit, in which case you
 * have sbrk try to reserve swap pages for each allocation, which will
 * fail for huge allocations. The problem with this in practice is
 * that you end up needing a huge swap disk even to run relatively
 * small workloads, and then this test takes forever to run.
 */
static
void
test9(void)
{
	size_t size;
	unsigned i, pages, dot;
	void *p;
	bool bad;

#define HUGESIZE (1024 * 1024 * 1024)	/* 1G */

	tprintf("Checking how much memory we can allocate:\n");
	for (size = HUGESIZE; (p = sbrk(size)) == (void *)-1; size = size/2) {
		tprintf("  %9lu bytes: failed\n", (unsigned long) size);
	}
	tprintf("  %9lu bytes: succeeded\n", (unsigned long) size);
	tprintf("Passed sbrk test 9 (part 1/5)\n");

	tprintf("Touching each page.\n");
	pages = size / PAGE_SIZE;
	dot = pages / 64;
	for (i=0; i<pages; i++) {
		markpagelight(p, i);
		if (dot > 0) {
			TEST161_LPROGRESS_N(i, dot);
		}
	}
	if (dot > 0) {
		printf("\n");
	}

	tprintf("Testing each page.\n");
	bad = false;
	for (i=0; i<pages; i++) {
		if (checkpagelight(p, i, dot > 0)) {
			if (dot > 0) {
				tprintf("\n");
			}
			warnx("FAILED: data corrupt");
			bad = true;
		}
		if (dot > 0) {
			TEST161_LPROGRESS_N(i, dot);
		}
	}
	if (dot > 0) {
		printf("\n");
	}
	if (bad) {
		exit(1);
	}
	tprintf("Passed sbrk test 9 (part 2/5)\n");

	tprintf("Freeing the memory.\n");
	(void)dosbrk(-size);
	tprintf("Passed sbrk test 9 (part 3/5)\n");

	tprintf("Allocating the memory again.\n");
	(void)dosbrk(size);
	tprintf("Passed sbrk test 9 (part 4/5)\n");

	tprintf("And really freeing it.\n");
	(void)dosbrk(-size);
	tprintf("Passed sbrk test 9 (all)\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocate all of memory one page at a time. The same restrictions
 * and considerations apply as above.
 */
static
void
test10(void)
{
	void *p, *op;
	unsigned i, n;
	bool bad;

	tprintf("Allocating all of memory one page at a time:\n");
	op = dosbrk(0);
	n = 0;
	while ((p = sbrk(PAGE_SIZE)) != (void *)-1) {
		markpagelight(op, n);
		n++;
	}
	tprintf("Got %u pages (%zu bytes).\n", n, (size_t)PAGE_SIZE * n);

	tprintf("Now freeing them.\n");
	bad = false;
	for (i=0; i<n; i++) {
		if (checkpagelight(op, n - i - 1, false)) {
			warnx("FAILED: data corrupt on page %u", i);
			bad = true;
		}
		(void)dosbrk(-PAGE_SIZE);
	}
	if (bad) {
		exit(1);
	}
	tprintf("Freed %u pages.\n", n);

	p = dosbrk(0);
	if (p != op) {
		errx(1, "FAILURE: break did not return to original value");
	}

	tprintf("Now let's see if I can allocate another page.\n");
	p = dosbrk(PAGE_SIZE);
	markpage(p, 0);
	if (checkpage(p, 0, false)) {
		errx(1, "FAILED: data corrupt");
	}
	(void)dosbrk(-PAGE_SIZE);

	tprintf("Passed sbrk test 10.\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

////////////////////////////////////////////////////////////
// leaking and cleanup on exit

static
void
test11(void)
{
	const unsigned num = 256;

	void *p;
	unsigned i;
	bool bad;

	tprintf("Allocating %u pages (%zu bytes).\n", num,
	       (size_t)PAGE_SIZE * num);
	p = dosbrk(num * PAGE_SIZE);

	tprintf("Touching the pages.\n");
	for (i=0; i<num; i++) {
		markpagelight(p, i);
		TEST161_LPROGRESS_N(i, 4);
	}
	tprintf("\n");

	tprintf("Checking the pages.\n");
	bad = false;
	for (i=0; i<num; i++) {
		if (checkpagelight(p, i, true)) {
			warnx("FAILED: data corrupt");
			bad = true;
		}
		TEST161_LPROGRESS_N(i, 4);
	}
	printf("\n");
	if (bad) {
		exit(1);
	}

	tprintf("Now NOT freeing the pages. They should get freed on exit.\n");
	tprintf("If not, you'll notice pretty quickly.\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

////////////////////////////////////////////////////////////
// forking

/*
 * Fork and then allocate in both the parent and the child, in case
 * fork messes up the heap. Note: this is not intended to test the
 * parent and child running concurrently -- that should probably be
 * its own test program.
 */
static
void
test12(void)
{
	pid_t pid;
	void *p;

	tprintf("Forking...\n");
	pid = dofork();
	if (pid == 0) {
		/* child */
		say("Child allocating a page...\n");
		p = dosbrk(PAGE_SIZE);
		markpage(p, 0);
		if (checkpage(p, 0, false)) {
			errx(1, "FAILED: data corrupt in child");
		}
		say("Child done.\n");
		exit(0);
	}
	/* parent */
	say("Parent allocating a page...\n");
	p = dosbrk(PAGE_SIZE);
	markpage(p, 0);
	if (checkpage(p, 0, false)) {
		errx(1, "FAILED: data corrupt in parent");
	}
	say("Parent done.\n");
	dowait(pid);
	tprintf("Passed sbrk test 12.\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocate and then fork, in case fork doesn't preserve the heap.
 */
static
void
test13(void)
{
	pid_t pid;
	void *p;

	tprintf("Allocating a page...\n");
	p = dosbrk(PAGE_SIZE);
	markpage(p, 0);
	if (checkpage(p, 0, false)) {
		errx(1, "FAILED: data corrupt before forking");
	}

	tprintf("Forking...\n");
	pid = dofork();
	if (pid == 0) {
		/* child */
		if (checkpage(p, 0, false)) {
			errx(1, "FAILED: data corrupt in child");
		}
		exit(0);
	}
	if (checkpage(p, 0, false)) {
		errx(1, "FAILED: data corrupt in parent");
	}
	dowait(pid);
	tprintf("Passed sbrk test 13.\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocate, then fork, then free the allocated page in the child.
 */
static
void
test14(void)
{
	pid_t pid;
	void *p;

	tprintf("Allocating a page...\n");
	p = dosbrk(PAGE_SIZE);
	markpage(p, 0);
	if (checkpage(p, 0, false)) {
		errx(1, "FAILED: data corrupt before forking");
	}

	tprintf("Forking...\n");
	pid = dofork();
	if (pid == 0) {
		/* child */
		if (checkpage(p, 0, false)) {
			errx(1, "FAILED: data corrupt in child");
		}
		tprintf("Child freeing a page...\n");
		dosbrk(-PAGE_SIZE);
		exit(0);
	}
	dowait(pid);
	if (checkpage(p, 0, false)) {
		errx(1, "FAILED: data corrupt in parent after child ran");
	}
	tprintf("Passed sbrk test 14.\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

/*
 * Allocate and free in both the parent and the child, and do more
 * than one page.
 */
static
void
test15(void)
{
	unsigned num = 12;

	pid_t pid;
	unsigned i;
	void *p;

	tprintf("Allocating %u pages...\n", num);
	p = dosbrk(PAGE_SIZE * num);
	for (i=0; i<num; i++) {
		markpage(p, i);
	}
	for (i=0; i<num; i++) {
		if (checkpage(p, i, false)) {
			errx(1, "FAILED: data corrupt before forking");
		}
	}

	tprintf("Freeing one page...\n");
	(void)dosbrk(-PAGE_SIZE);
	num--;
	for (i=0; i<num; i++) {
		if (checkpage(p, i, false)) {
			errx(1, "FAILED: data corrupt before forking (2)");
		}
	}

	tprintf("Allocating two pages...\n");
	(void)dosbrk(PAGE_SIZE * 2);
	markpage(p, num++);
	markpage(p, num++);
	for (i=0; i<num; i++) {
		if (checkpage(p, i, false)) {
			errx(1, "FAILED: data corrupt before forking (3)");
		}
	}

	tprintf("Forking...\n");
	pid = dofork();
	if (pid == 0) {
		/* child */
		for (i=0; i<num; i++) {
			if (checkpage(p, i, false)) {
				errx(1, "FAILED: data corrupt in child");
			}
		}

		say("Child: freeing three pages\n");
		dosbrk(-PAGE_SIZE * 3);
		num -= 3;
		for (i=0; i<num; i++) {
			if (checkpage(p, i, false)) {
				errx(1, "FAILED: data corrupt in child (2)");
			}
		}

		say("Child: allocating two pages\n");
		dosbrk(PAGE_SIZE * 2);
		markpage(p, num++);
		markpage(p, num++);
		for (i=0; i<num; i++) {
			if (checkpage(p, i, false)) {
				errx(1, "FAILED: data corrupt in child (3)");
			}
		}

		say("Child: freeing all\n");
		dosbrk(-PAGE_SIZE * num);
		exit(0);
	}
	say("Parent: allocating four pages\n");
	dosbrk(PAGE_SIZE * 4);
	for (i=0; i<4; i++) {
		markpage(p, num++);
	}
	for (i=0; i<num; i++) {
		if (checkpage(p, i, false)) {
			errx(1, "FAILED: data corrupt in parent");
		}
	}

	say("Parent: waiting\n");
	dowait(pid);

	for (i=0; i<num; i++) {
		if (checkpage(p, i, false)) {
			errx(1, "FAILED: data corrupt after waiting");
		}
	}

	(void)dosbrk(-PAGE_SIZE * num);
	tprintf("Passed sbrk test 15.\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");

}

////////////////////////////////////////////////////////////
// stress testing

static
void
stresstest(unsigned long seed, bool large)
{
	const unsigned loops = 10000;
	const unsigned dot = 200;

	void *op;
	unsigned num;
	unsigned i, j, pages;
	unsigned long r;
	bool bad, neg;

	srandom(seed);
	tprintf("Seeded random number generator with %lu.\n", seed);

	op = dosbrk(0);

	bad = false;
	num = 0;
	for (i=0; i<loops; i++) {
		/*
		 * The goal of this test is not to stress the VM system
		 * by thrashing swap (other tests do that) but by running
		 * the sbrk code a lot. So, clamp the total size at either
		 * 128K or 512K, which is 32 or 128 pages respectively.
		 */
		r = random();
		pages = r % (large ? 32 : 8);
		neg = pages <= num && ((r & 128) == 0);
		if (!neg && num + pages > (large ? 128 : 32)) {
			neg = 1;
		}
		if (neg) {
			dosbrk(-(pages * PAGE_SIZE));
			num -= pages;
		}
		else {
			dosbrk(pages * PAGE_SIZE);
			for (j=0; j<pages; j++) {
				markpagelight(op, num + j);
			}
			num += pages;
		}
		for (j=0; j<num; j++) {
			if (checkpagelight(op, j, true)) {
				tprintf("\n");
				warnx("FAILED: data corrupt on page %u", j);
				bad = true;
			}
		}
		TEST161_LPROGRESS_N(i, dot);
	}
	printf("\n");
	if (bad) {
		warnx("FAILED");
		exit(1);
	}

	dosbrk(-(num * PAGE_SIZE));
	tprintf("Passed sbrk %s stress test.\n", large ? "large" : "small");
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

static
void
test16(void)
{
	stresstest(0, false);
}

static
void
test17(void)
{
	stresstest(getseed(), false);
}

static
void
test18(void)
{
	tprintf("Enter random seed: ");
	stresstest(geti(), false);
}

static
void
test19(void)
{
	stresstest(0, true);
}

static
void
test20(void)
{
	stresstest(getseed(), true);
}

static
void
test21(void)
{
	tprintf("Enter random seed: ");
	stresstest(geti(), true);
}

static
void
test22(void)
{
	int i;
	void *p, *q;
	int num = 10;
	int num_pages = 5 * 1024;	// 20MB

	p = dosbrk(num_pages * PAGE_SIZE);
	q = dosbrk(0);

	if ((unsigned int)q - (unsigned int)p != (unsigned int)(num_pages*PAGE_SIZE)) {
		errx(1, "Heap size not equal to expected size: p=0x%x q=0x%x", (unsigned int)p, (unsigned int)q);
	}

	// Just touch the last 10 pages
	for (i = 0; i < num; i++) {
		markpage(p, num_pages-(i+1));
	}

	// Check the last 10 pages
	for (i = 0; i < num; i++) {
		if (checkpage(p, num_pages-(i+1), false)) {
			errx(1, "FAILED: data corrupt");
		}
	}

	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

static
void
test23(void)
{
	// Make sure sbrk is freeing memory. This allocates, in total, just over 4M
	// of memory, but moves the heap breakpoint in such a way that only one page
	// should ever be required. This test doesn't make much sense to run with
	// more than 4M or with swap enabled.
	void *start;
	int num_pages = 1030;
	int num;

	start = dosbrk(PAGE_SIZE);

	for (num = 1; num <= num_pages; num++) {
		TEST161_LPROGRESS(num);
		start = dosbrk(num*PAGE_SIZE);
		markpagelight(start, num-1);
		checkpagelight(start, num-1, true);
		dosbrk(-(num*PAGE_SIZE));
	}
	success(TEST161_SUCCESS, SECRET, "/testbin/sbrktest");
}

////////////////////////////////////////////////////////////
// main

static const struct {
	int num;
	const char *desc;
	void (*func)(void);
} tests[] = {
	{ 1, "Allocate one page", test1 },
	{ 2, "Allocate and free one page", test2 },
	{ 3, "Allocate and free several pages", test3 },
	{ 4, "Allocate several pages and free them one at a time", test4 },
	{ 5, "Check the heap end (crashes)", test5 },
	{ 6, "Allocate and check the heap end (crashes)", test6 },
	{ 7, "Allocate and free and check the heap end (crashes)", test7 },
	{ 8, "Allocate several, free some, check heap end (crashes)", test8 },
	{ 9, "Allocate all memory in a big chunk", test9 },
	{ 10, "Allocate all memory a page at a time", test10 },
	{ 11, "Allocate a lot and intentionally leak it", test11 },
	{ 12, "Fork and then allocate", test12 },
	{ 13, "Allocate and then fork", test13 },
	{ 14, "Allocate and then fork and free", test14 },
	{ 15, "Allocate, fork, allocate more, and free (and spam)", test15 },
	{ 16, "Small stress test", test16 },
	{ 17, "Randomized small stress test", test17 },
	{ 18, "Small stress test with particular seed", test18 },
	{ 19, "Large stress test", test19 },
	{ 20, "Randomized large stress test", test20 },
	{ 21, "Large stress test with particular seed", test21 },
	{ 22, "Large sbrk test", test22 },
	{ 23, "Allocate 4MB in total, but free pages in between", test23 },
};
static const unsigned numtests = sizeof(tests) / sizeof(tests[0]);

static
int
dotest(int tn)
{
	unsigned i;

	for (i=0; i<numtests; i++) {
		if (tests[i].num == tn) {
			tests[i].func();
			return 0;
		}
	}
	return -1;
}

int
main(int argc, char *argv[])
{
	int i, tn;
	unsigned j;
	bool menu = true;

	setup();

	if (argc > 1) {
		for (i=1; i<argc; i++) {
			dotest(atoi(argv[i]));
		}
		return 0;
	}

	while (1) {
		if (menu) {
			for (j=0; j<numtests; j++) {
				tprintf("  %2d  %s\n", tests[j].num,
				       tests[j].desc);
			}
			menu = false;
		}
		tprintf("sbrktest: ");
		tn = geti();
		if (tn < 0) {
			break;
		}

		if (dotest(tn)) {
			menu = true;
		}
	}

	return 0;
}
