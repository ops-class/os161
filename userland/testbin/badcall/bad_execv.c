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
 * bad calls to execv()
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#include "config.h"
#include "test.h"

static
int
exec_common_fork(void)
{
	int pid, rv, status, err;

	/*
	 * This does not happen in a test context (from the point of
	 * view of report.c) so we have to fiddle a bit.
	 */

	pid = fork();
	if (pid<0) {
		err = errno;
		report_begin("forking for test");
		report_result(pid, err);
		report_aborted();
		return -1;
	}

	if (pid==0) {
		/* child */
		return 0;
	}

	rv = waitpid(pid, &status, 0);
	if (rv == -1) {
		err = errno;
		report_begin("waiting for test subprocess");
		report_result(rv, err);
		report_failure();
		return -1;
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) == MAGIC_STATUS) {
		return 1;
	}
	/* Oops... */
	report_begin("exit code of subprocess; should be %d", MAGIC_STATUS);
	if (WIFSIGNALED(status)) {
		report_warnx("signal %d", WTERMSIG(status));
	}
	else {
		report_warnx("exit %d", WEXITSTATUS(status));
	}
	report_failure();
	return -1;
}

static
void
exec_badprog(const void *prog, const char *desc)
{
	int rv;
	char *args[2];
	args[0] = (char *)"foo";
	args[1] = NULL;

	if (exec_common_fork() != 0) {
		return;
	}

	report_begin(desc);
	rv = execv(prog, args);
	report_check(rv, errno, EFAULT);
	exit(MAGIC_STATUS);
}

static
void
exec_emptyprog(void)
{
	int rv;
	char *args[2];
	args[0] = (char *)"foo";
	args[1] = NULL;

	if (exec_common_fork() != 0) {
		return;
	}

	report_begin("exec the empty string");
	rv = execv("", args);
	report_check2(rv, errno, EINVAL, EISDIR);
	exit(MAGIC_STATUS);
}

static
void
exec_badargs(void *args, const char *desc)
{
	int rv;

	if (exec_common_fork() != 0) {
		return;
	}

	report_begin(desc);
	rv = execv("/bin/true", args);
	report_check(rv, errno, EFAULT);
	exit(MAGIC_STATUS);
}

static
void
exec_onearg(void *ptr, const char *desc)
{
	int rv;

	char *args[3];
	args[0] = (char *)"foo";
	args[1] = (char *)ptr;
	args[2] = NULL;

	if (exec_common_fork() != 0) {
		return;
	}

	report_begin(desc);
	rv = execv("/bin/true", args);
	report_check(rv, errno, EFAULT);
	exit(MAGIC_STATUS);
}

void
test_execv(void)
{
	exec_badprog(NULL, "exec with NULL program");
	exec_badprog(INVAL_PTR, "exec with invalid pointer program");
	exec_badprog(KERN_PTR, "exec with kernel pointer program");

	exec_emptyprog();

	exec_badargs(NULL, "exec with NULL arglist");
	exec_badargs(INVAL_PTR, "exec with invalid pointer arglist");
	exec_badargs(KERN_PTR, "exec with kernel pointer arglist");

	exec_onearg(INVAL_PTR, "exec with invalid pointer arg");
	exec_onearg(KERN_PTR, "exec with kernel pointer arg");
}
