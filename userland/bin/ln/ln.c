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
#include <stdlib.h>
#include <string.h>
#include <err.h>

/*
 * ln - hardlink or symlink files
 *
 * Usage: ln oldfile newfile
 *        ln -s symlinkcontents symlinkfile
 */


/*
 * Create a symlink with filename PATH that contains text TEXT.
 * When fed to ls -l, this produces something that looks like
 *
 * lrwxrwxrwx  [stuff]   PATH -> TEXT
 */
static
void
dosymlink(const char *text, const char *path)
{
	if (symlink(text, path)) {
		err(1, "%s", path);
	}
}

/*
 * Create a hard link such that NEWFILE names the same file as
 * OLDFILE. Since it's a hard link, the two names for the file
 * are equal; both are the "real" file.
 */
static
void
dohardlink(const char *oldfile, const char *newfile)
{
	if (link(oldfile, newfile)) {
		err(1, "%s or %s", oldfile, newfile);
		exit(1);
	}
}

int
main(int argc, char *argv[])
{
	/*
	 * Just do whatever was asked for.
	 *
	 * We don't allow the Unix model where you can do
	 *    ln [-s] file1 file2 file3 destination-directory
	 */
	if (argc==4 && !strcmp(argv[1], "-s")) {
		dosymlink(argv[2], argv[3]);
	}
	else if (argc==3) {
		dohardlink(argv[1], argv[2]);
	}
	else {
		warnx("Usage: ln oldfile newfile");
		errx(1, "       ln -s symlinkcontents symlinkfile\n");
	}
	return 0;
}
