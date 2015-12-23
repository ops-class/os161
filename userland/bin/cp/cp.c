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
#include <err.h>

/*
 * cp - copy a file.
 * Usage: cp oldfile newfile
 */


/* Copy one file to another. */
static
void
copy(const char *from, const char *to)
{
	int fromfd;
	int tofd;
	char buf[1024];
	int len, wr, wrtot;

	/*
	 * Open the files, and give up if they won't open
	 */
	fromfd = open(from, O_RDONLY);
	if (fromfd<0) {
		err(1, "%s", from);
	}
	tofd = open(to, O_WRONLY|O_CREAT|O_TRUNC);
	if (tofd<0) {
		err(1, "%s", to);
	}

	/*
	 * As long as we get more than zero bytes, we haven't hit EOF.
	 * Zero means EOF. Less than zero means an error occurred.
	 * We may read less than we asked for, though, in various cases
	 * for various reasons.
	 */
	while ((len = read(fromfd, buf, sizeof(buf)))>0) {
		/*
		 * Likewise, we may actually write less than we attempted
		 * to. So loop until we're done.
		 */
		wrtot = 0;
		while (wrtot < len) {
			wr = write(tofd, buf+wrtot, len-wrtot);
			if (wr<0) {
				err(1, "%s", to);
			}
			wrtot += wr;
		}
	}
	/*
	 * If we got a read error, print it and exit.
	 */
	if (len<0) {
		err(1, "%s", from);
	}

	if (close(fromfd) < 0) {
		err(1, "%s: close", from);
	}

	if (close(tofd) < 0) {
		err(1, "%s: close", to);
	}
}

int
main(int argc, char *argv[])
{
	/*
	 * Just do it.
	 *
	 * We don't allow the Unix model where you can do
	 *    cp file1 file2 file3 destination-directory
	 *
	 * although this would be pretty easy to add.
	 */
	if (argc!=3) {
		errx(1, "Usage: cp OLDFILE NEWFILE");
	}
	copy(argv[1], argv[2]);
	return 0;
}
