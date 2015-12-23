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

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*
 * system(): ANSI C
 *
 * Run a command.
 */

#define MAXCMDSIZE 2048
#define MAXARGS    128

int
system(const char *cmd)
{
	/*
	 * Ordinarily, you call the shell to process the command.
	 * But we don't know that the shell can do that. So, do it
	 * ourselves.
	 */

	char tmp[MAXCMDSIZE];
	char *argv[MAXARGS+1];
	int nargs=0;
	char *s;
	int pid, status;

	if (strlen(cmd) >= sizeof(tmp)) {
		errno = E2BIG;
		return -1;
	}
	strcpy(tmp, cmd);

	for (s = strtok(tmp, " \t"); s; s = strtok(NULL, " \t")) {
		if (nargs < MAXARGS) {
			argv[nargs++] = s;
		}
		else {
			errno = E2BIG;
			return 1;
		}
	}

	argv[nargs] = NULL;

	pid = fork();
	switch (pid) {
	    case -1:
		return -1;
	    case 0:
		/* child */
		execv(argv[0], argv);
		/* exec only returns if it fails */
		_exit(255);
	    default:
		/* parent */
		waitpid(pid, &status, 0);
		return status;
	}
}
