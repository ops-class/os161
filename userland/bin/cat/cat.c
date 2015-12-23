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

#include <unistd.h>
#include <string.h>
#include <err.h>

/*
 * cat - concatenate and print
 * Usage: cat [files]
 */



/* Print a file that's already been opened. */
static
void
docat(const char *name, int fd)
{
	char buf[1024];
	int len, wr, wrtot;

	/*
	 * As long as we get more than zero bytes, we haven't hit EOF.
	 * Zero means EOF. Less than zero means an error occurred.
	 * We may read less than we asked for, though, in various cases
	 * for various reasons.
	 */
	while ((len = read(fd, buf, sizeof(buf)))>0) {
		/*
		 * Likewise, we may actually write less than we attempted
		 * to. So loop until we're done.
		 */
		wrtot = 0;
		while (wrtot < len) {
			wr = write(STDOUT_FILENO, buf+wrtot, len-wrtot);
			if (wr<0) {
				err(1, "stdout");
			}
			wrtot += wr;
		}
	}
	/*
	 * If we got a read error, print it and exit.
	 */
	if (len<0) {
		err(1, "%s", name);
	}
}

/* Print a file by name. */
static
void
cat(const char *file)
{
	int fd;

	/*
	 * "-" means print stdin.
	 */
	if (!strcmp(file, "-")) {
		docat("stdin", STDIN_FILENO);
		return;
	}

	/*
	 * Open the file, print it, and close it.
	 * Bail out if we can't open it.
	 */
	fd = open(file, O_RDONLY);
	if (fd<0) {
		err(1, "%s", file);
	}
	docat(file, fd);
	close(fd);
}


int
main(int argc, char *argv[])
{
	if (argc==1) {
		/* No args - just do stdin */
		docat("stdin", STDIN_FILENO);
	}
	else {
		/* Print all the files specified on the command line. */
		int i;
		for (i=1; i<argc; i++) {
			cat(argv[i]);
		}
	}
	return 0;
}
