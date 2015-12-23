/*
 * Copyright (c) 2013
 *	The President and Fellows of Harvard College.
 *      Written by David A. Holland.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "workloads.h"
#include "main.h"

struct workload {
	const char *name;
	const char *argname;
	union {
		void (*witharg)(const char *);
		void (*noarg)(void);
	} run;
};

#define WL(n)    { .name = #n, .argname = NULL, .run.noarg = wl_##n }
#define WLA(n,a) { .name = #n, .argname = #a,   .run.witharg = wl_##n }

static const struct workload workloads[] = {
	WLA(createwrite, size),
	WLA(rewrite, size),
	WLA(randupdate, size),
	WLA(truncwrite, size),
	WLA(makehole, size),
	WLA(fillhole, size),
	WLA(truncfill, size),
	WLA(append, size),
	WLA(trunczero, size),
	WLA(trunconeblock, size),
	WLA(truncsmallersize, size),
	WLA(trunclargersize, size),
	WLA(appendandtrunczero, size),
	WLA(appendandtruncpartly, size),
	WL(mkfile),
	WL(mkdir),
	WL(mkmanyfile),
	WL(mkmanydir),
	WL(mktree),
	WLA(mkrandtree, seed),
	WL(rmfile),
	WL(rmdir),
	WL(rmfiledelayed),
	WL(rmfiledelayedappend),
	WL(rmdirdelayed),
	WL(rmmanyfile),
	WL(rmmanyfiledelayed),
	WL(rmmanyfiledelayedandappend),
	WL(rmmanydir),
	WL(rmmanydirdelayed),
	WL(rmtree),
	WLA(rmrandtree, seed),
	WL(linkfile),
	WL(linkmanyfile),
	WL(unlinkfile),
	WL(unlinkmanyfile),
	WL(linkunlinkfile),
	WL(renamefile),
	WL(renamedir),
	WL(renamesubtree),
	WL(renamexdfile),
	WL(renamexddir),
	WL(renamexdsubtree),
	WL(renamemanyfile),
	WL(renamemanydir),
	WL(renamemanysubtree),
	WL(copyandrename),
	WL(untar),
	WL(compile),
	WL(cvsupdate),
	WLA(writefileseq, seed),
	WLA(writetruncseq, seed),
	WLA(mkrmseq, seed),
	WLA(linkunlinkseq, seed),
	WLA(renameseq, seed),
	WLA(diropseq, seed),
	WLA(genseq, seed),
};
static const unsigned numworkloads = sizeof(workloads) / sizeof(workloads[0]);

#undef WL
#undef WLA

static
const struct workload *
findworkload(const char *name)
{
	unsigned i;

	for (i=0; i<numworkloads; i++) {
		if (!strcmp(workloads[i].name, name)) {
			return &workloads[i];
		}
	}
	return NULL;
}

static
void
printworkloads(void)
{
	unsigned i;

	printf("Supported workloads:\n");
	for (i=0; i<numworkloads; i++) {
		printf("   %s", workloads[i].name);
		if (workloads[i].argname) {
			printf(" %s", workloads[i].argname);
		}
		printf("\n");
	}
}

int
main(int argc, char *argv[])
{
	const char *workloadname;
	const struct workload *workload;
	int checkmode = 0;

	if (argc == 2 && !strcmp(argv[1], "list")) {
		printworkloads();
		exit(0);
	}

	if (argc < 3) {
		warnx("Usage: %s do|check workload [arg]", argv[0]);
		warnx("Use \"list\" for a list of workloads");
		exit(1);
	}

	if (!strcmp(argv[1], "do")) {
		checkmode = 0;
	}
	else if (!strcmp(argv[1], "check")) {
		checkmode = 1;
	}
	else {
		errx(1, "Action must be \"do\" or \"check\"");
	}

	workloadname = argv[2];
	workload = findworkload(workloadname);
	if (workload == NULL) {
		errx(1, "Unknown workload %s\n", workloadname);
		printworkloads();
		exit(1);
	}
	setcheckmode(checkmode);
	if (workload->argname) {
		if (argc != 4) {
			errx(1, "%s requires argument %s\n",
			     workloadname, workload->argname);
		}
		workload->run.witharg(argv[3]);
	}
	else {
		if (argc != 3) {
			errx(1, "Stray argument for workload %s",workloadname);
		}
		workload->run.noarg();
	}
	complete();
	return 0;
}
