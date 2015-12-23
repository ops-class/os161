/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2009, 2013
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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <err.h>

#include "compat.h"

#include "disk.h"
#include "sfs.h"
#include "sb.h"
#include "freemap.h"
#include "inode.h"
#include "passes.h"
#include "main.h"

static int badness=0;

/*
 * Update the badness state. (codes are in main.h)
 *
 * The badness state only gets worse, and is ultimately the process
 * exit code.
 */
void
setbadness(int code)
{
	if (badness < code) {
		badness = code;
	}
}

/*
 * Main.
 */
int
main(int argc, char **argv)
{
#ifdef HOST
	hostcompat_init(argc, argv);
#endif

	/* FUTURE: add -n option */
	if (argc!=2) {
		errx(EXIT_USAGE, "Usage: sfsck device/diskfile");
	}

	opendisk(argv[1]);

	sfs_setup();
	sb_load();
	sb_check();
	freemap_setup();

	printf("Phase 1 -- check blocks and sizes\n");
	pass1();
	freemap_check();

	printf("Phase 2 -- check directory tree\n");
	inode_sorttable();
	pass2();

	printf("Phase 3 -- check reference counts\n");
	inode_adjust_filelinks();

	closedisk();

	warnx("%lu blocks used (of %lu); %lu directories; %lu files",
	      freemap_blocksused(), (unsigned long)sb_totalblocks(),
	      pass1_founddirs(), pass1_foundfiles());

	switch (badness) {
	    case EXIT_USAGE:
	    case EXIT_FATAL:
	    default:
		/* not supposed to happen here */
		assert(0);
		break;
	    case EXIT_UNRECOV:
		warnx("WARNING - unrecoverable errors. Maybe try again?");
		break;
	    case EXIT_RECOV:
		warnx("Caution - filesystem modified. Run again for luck.");
		break;
	    case EXIT_CLEAN:
		break;
	}

	return badness;
}
