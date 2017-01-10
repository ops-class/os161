/*
 * Copyright (c) 2015
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
 * bigfork - concurrent VM test that behaves somewhat better than
 * parallelvm.
 *
 * This test is a mixture of forktest and parallelvm: it does nested
 * forks like forktest, and aimless matrix operations like parallelvm;
 * the goal is to serve as a performance benchmark more than as a
 * stress test (though it can be that too) and in particular to
 * exhibit less timing variance than parallelvm does. The variance is
 * still fairly high, but the variance of parallelvm is horrific.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#define BRANCHES 6

/*
 * 6 branches gives 64 procs at the final stage, and we want this to
 * use about 4M. So each proc's memory load should be about 1/16M or
 * 64K. Which is 16384 ints, or four 64x64 matrixes.
 */
#define DIM 64

static int m1[DIM*DIM], m2[DIM*DIM], m3[DIM*DIM], m4[DIM*DIM];
static const int right[BRANCHES] = {
	536763422,
	478946723,
	375722852,
	369910585,
	328220902,
	62977821,
};
static unsigned failures;

static
void
init(void)
{
	unsigned i, j;

	srandom(73771);
	for (i=0; i<DIM; i++) {
		for (j=0; j<DIM; j++) {
			m1[i*DIM+j] = random() % 11 - 5;
		}
	}
}

static
void
add(int *x, const int *a, const int *b)
{
	unsigned i, j;

	for (i=0; i<DIM; i++) {
		for (j=0; j<DIM; j++) {
			x[i*DIM+j] = a[i*DIM+j] + b[i*DIM+j];
		}
	}
}

static
void
mul(int *x, const int *a, const int *b)
{
	unsigned i, j, k;

	for (i=0; i<DIM; i++) {
		for (j=0; j<DIM; j++) {
			x[i*DIM+j] = 0;
			for (k=0; k<DIM; k++) {
				x[i*DIM+j] += a[i*DIM+k] * b[k*DIM+j]; 
			}
		}
	}
}

static
void
scale(int *x, const int *a, int b)
{
	unsigned i, j;

	for (i=0; i<DIM; i++) {
		for (j=0; j<DIM; j++) {
			x[i*DIM+j] = a[i*DIM+j] / b;
		}
	}
}

static
void
grind(void)
{
	/*
	 * compute: m2 = m1*m1, m3 = m2+m1, m4 = m3*m3, m1 = m4 / 2
	 */
	 mul(m2, m1, m1);
	 add(m3, m2, m1);
	 mul(m4, m3, m3);
	 scale(m1, m4, 2);
}

static
int
trace(void)
{
	unsigned i;
	int val = 0;

	for (i=0; i<DIM; i++) {
		val += m1[i*DIM+i];
	}
	while (val < 0) {
		val += 0x20000000;
	}
	return val % 0x20000000;
}

static
pid_t
dofork(void)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		warn("fork");
	}
	return pid;
}

static
void
dowait(pid_t pid)
{
	int status;

	if (pid == -1) {
		failures++;
		return;
	}
	if (pid == 0) {
		exit(failures);
	}
	else {
		if (waitpid(pid, &status, 0) < 0) {
			warn("waitpid(%d)", pid);
		}
		else if (WIFSIGNALED(status)) {
			warnx("pid %d: signal %d", pid, WTERMSIG(status));
		}
		else if (WEXITSTATUS(status) > 0) {
			failures += WEXITSTATUS(status);
		}
	}
}

static
void
dotest(void)
{
	unsigned i, me;
	pid_t pids[BRANCHES];
	int t;
	char msg[128];

	me = 0;
	for (i=0; i<BRANCHES; i++) {
		pids[i] = dofork();
		if (pids[i] == 0) {
			me += 1U<<i;
		}
		grind();
		t = trace();
		if (t == right[i]) {
			snprintf(msg, sizeof(msg),
				 "Stage %u #%u done: %d\n", i, me, trace());
		}
		else {
			snprintf(msg, sizeof(msg),
				 "Stage %u #%u FAILED: got %d, expected %d\n",
				 i, me, t, right[i]);
			failures++;
		}
		(void)write(STDOUT_FILENO, msg, strlen(msg));
	}

	for (i=BRANCHES; i-- > 0; ) {
		dowait(pids[i]);
	}
	if (failures > 0) {
		printf("%u failures.\n", failures);
	}
	else {
		printf("Done.\n");
	}
}

int
main(void)
{
	init();
	dotest();
	return 0;
}
