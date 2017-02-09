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
 * parallelvm.c: highly parallelized VM stress test.
 *
 * This test probably won't run with only 512k of physical memory
 * (unless maybe if you have a *really* gonzo VM system) because each
 * of its processes needs to allocate a kernel stack, and those add up
 * quickly.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#define NJOBS    24

#define DIM      35
#define NMATS    11
#define JOBSIZE  ((NMATS+1)*DIM*DIM*sizeof(int))

static const int right_answers[NJOBS] = {
        -1337312809,
	356204544,
	-537881911,
	-65406976,
	1952063315,
	-843894784,
	1597000869,
	-993925120,
	838840559,
        -1616928768,
	-182386335,
	-364554240,
	251084843,
	-61403136,
	295326333,
	1488013312,
	1901440647,
	0,
        -1901440647,
        -1488013312,
	-295326333,
	61403136,
	-251084843,
	364554240,
};

////////////////////////////////////////////////////////////

struct matrix {
	int m_data[DIM][DIM];
};

////////////////////////////////////////////////////////////

/*
 * Use this instead of just calling printf so we know each printout
 * is atomic; this prevents the lines from getting intermingled.
 */
static
void
say(const char *fmt, ...)
{
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	write(STDOUT_FILENO, buf, strlen(buf));
}

////////////////////////////////////////////////////////////

static
void
multiply(struct matrix *res, const struct matrix *m1, const struct matrix *m2)
{
	int i, j, k;

	for (i=0; i<DIM; i++) {
		for (j=0; j<DIM; j++) {
			int val=0;
			for (k=0; k<DIM; k++) {
				val += m1->m_data[i][k]*m2->m_data[k][j];
			}
			res->m_data[i][j] = val;
		}
	}
}

static
void
addeq(struct matrix *m1, const struct matrix *m2)
{
	int i, j;
	for (i=0; i<DIM; i++) {
		for (j=0; j<DIM; j++) {
			m1->m_data[i][j] += m2->m_data[i][j];
		}
	}
}

static
int
trace(const struct matrix *m1)
{
	int i, t=0;
	for (i=0; i<DIM; i++) {
		t += m1->m_data[i][i];
	}
	return t;
}

////////////////////////////////////////////////////////////

static struct matrix mats[NMATS];

static
void
populate_initial_matrixes(int mynum)
{
	int i,j;
	struct matrix *m = &mats[0];
	for (i=0; i<DIM; i++) {
		for (j=0; j<DIM; j++) {
			m->m_data[i][j] = mynum+i-2*j;
		}
	}

	multiply(&mats[1], &mats[0], &mats[0]);
}

static
void
compute(int n)
{
	struct matrix tmp;
	int i, j;

	for (i=0,j=n-1; i<j; i++,j--) {
		multiply(&tmp, &mats[i], &mats[j]);
		addeq(&mats[n], &tmp);
	}
}

static
void
computeall(int mynum)
{
	int i;
	populate_initial_matrixes(mynum);
	for (i=2; i<NMATS; i++) {
		compute(i);
	}
}

static
int
answer(void)
{
	return trace(&mats[NMATS-1]);
}

static
void
go(int mynum)
{
	int r;

	say("Process %d (pid %d) starting computation...\n", mynum,
	    (int) getpid());

	computeall(mynum);
	r = answer();

	if (r != right_answers[mynum]) {
		say("Process %d answer %d: FAILED, should be %d\n",
		    mynum, r, right_answers[mynum]);
		exit(1);
	}
	say("Process %d answer %d: passed\n", mynum, r);
	exit(0);
}

////////////////////////////////////////////////////////////
// semaphores

/*
 * We open the semaphore separately in each process to avoid
 * filehandle-level locking problems. If you can't be "reading" and
 * "writing" the semaphore concurrently because of the open file
 * object lock, then using the same file handle for P and V will
 * deadlock. Also, if this same lock is used to protect the reference
 * count on the open file logic, fork will block if another process is
 * using the same file handle for P, and then we're deadlocked too.
 *
 * Ideally the open file / filetable code wouldn't have this problem,
 * as it makes e.g. console output from background jobs behave
 * strangely, but it's a common issue in practice and it's better for
 * parallelvm to be immune to it.
 */

struct usem {
	char name[32];
	int fd;
};

static
void
semcreate(const char *tag, struct usem *sem)
{
	int fd;

	snprintf(sem->name, sizeof(sem->name), "sem:parallelvm.%s.%d",
		 tag, (int)getpid());

	fd = open(sem->name, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	if (fd < 0) {
		err(1, "%s: create", sem->name);
	}
	close(fd);
}

static
void
semopen(struct usem *sem)
{
	sem->fd = open(sem->name, O_RDWR, 0664);
	if (sem->fd < 0) {
		err(1, "%s: open", sem->name);
	}
}

static
void
semclose(struct usem *sem)
{
	close(sem->fd);
}

static
void
semdestroy(struct usem *sem)
{
	remove(sem->name);
}

static
void
semP(struct usem *sem, size_t num)
{
	char c[num];

	if (read(sem->fd, c, num) < 0) {
		err(1, "%s: read", sem->name);
	}
	(void)c;
}

static
void
semV(struct usem *sem, size_t num)
{
	char c[num];

	/* semfs does not use these values, but be conservative */
	memset(c, 0, num);

	if (write(sem->fd, c, num) < 0) {
		err(1, "%s: write", sem->name);
	}
}

////////////////////////////////////////////////////////////
// driver

static
int
status_is_failure(int status)
{
	/* Proper interpretation of Unix exit status */
	if (WIFSIGNALED(status)) {
		return 1;
	}
	if (!WIFEXITED(status)) {
		/* ? */
		return 1;
	}
	status = WEXITSTATUS(status);
	return status != 0;
}

static
void
makeprocs(bool dowait)
{
	int i, status, failcount;
	struct usem s1, s2;
	pid_t pids[NJOBS];

	if (dowait) {
		semcreate("1", &s1);
		semcreate("2", &s2);
	}

	printf("Job size approximately %lu bytes\n", (unsigned long) JOBSIZE);
	printf("Forking %d jobs; total load %luk\n", NJOBS,
	       (unsigned long) (NJOBS * JOBSIZE)/1024);

	for (i=0; i<NJOBS; i++) {
		pids[i] = fork();
		if (pids[i]<0) {
			warn("fork (process %d)", i);
			if (dowait) {
				semopen(&s1);
				semV(&s1, 1);
				semclose(&s1);
			}
		}
		if (pids[i]==0) {
			/* child */
			if (dowait) {
				say("Process %d forked\n", i);
				semopen(&s1);
				semopen(&s2);
				semV(&s1, 1);
				semP(&s2, 1);
				semclose(&s1);
				semclose(&s2);
			}
			go(i);
		}
	}

	if (dowait) {
		semopen(&s1);
		semopen(&s2);
		say("Waiting for fork...\n");
		semP(&s1, NJOBS);
		say("Starting computation.\n");
		semV(&s2, NJOBS);
	}

	failcount=0;
	for (i=0; i<NJOBS; i++) {
		if (pids[i]<0) {
			failcount++;
		}
		else {
			if (waitpid(pids[i], &status, 0)<0) {
				err(1, "waitpid");
			}
			if (status_is_failure(status)) {
				failcount++;
			}
		}
	}

	if (failcount>0) {
		printf("%d subprocesses failed\n", failcount);
		exit(1);
	}
	printf("Test complete\n");

	semclose(&s1);
	semclose(&s2);
	semdestroy(&s1);
	semdestroy(&s2);
}

int
main(int argc, char *argv[])
{
	bool dowait = false;

	if (argc == 0) {
		/* broken/unimplemented argv handling; do nothing */
	}
	else if (argc == 1) {
		/* nothing */
	}
	else if (argc == 2 && !strcmp(argv[1], "-w")) {
		dowait = true;
	}
	else {
		printf("Usage: parallelvm [-w]\n");
		return 1;
	}
	makeprocs(dowait);
	return 0;
}
