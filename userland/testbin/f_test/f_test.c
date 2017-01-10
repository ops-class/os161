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
 * Razvan Surdulescu
 * abhi shelat
 * April 28 1997
 *
 * Test suite for Nachos HW4--The Filesystem
 *
 * Modified by dholland 1/31/2001 for OS/161
 *
 * This should run successfully (on SFS) when the file system
 * assignment is complete.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include "f_hdr.h"

#define SECTOR_SIZE  512


#define BUFFER_SIZE  (2 * SECTOR_SIZE + 1)
#define BIGFILE_SIZE (270 * BUFFER_SIZE)
#define BIGFILE_NAME "large-f"

#define LETTER(x) ('a' + (x % 31))

char fbuffer[BUFFER_SIZE];
char ibuffer[32];


#define DIR_DEPTH      8
#define DIR_NAME       "/t"
#define DIRFILE_NAME   "a"


#define FNAME        "f-testfile"
#define TMULT        50
#define FSIZE        ((SECTOR_SIZE + 1) * TMULT)

#define READCHAR     'r'
#define WRITECHAR    'w'

char cbuffer[SECTOR_SIZE + 1];


/* ===================================================

 */

static
pid_t
forkoff(void (*func)(void))
{
	pid_t pid = fork();
	switch (pid) {
	    case -1:
		warn("fork");
		return -1;
	    case 0:
		func();
		_exit(0);
	    default: break;
	}
	return pid;
}

static
void
dowait(int pid)
{
	int status;

	if (waitpid(pid, &status, 0)<0) {
		warn("waitpid for %d", pid);
	}
	else if (WIFSIGNALED(status)) {
		warnx("pid %d: signal %d", pid, WTERMSIG(status));
	}
	else if (WEXITSTATUS(status) != 0) {
		warnx("pid %d: exit %d", pid, WEXITSTATUS(status));
	}
}

/* ===================================================

 */

static
void
big_file(int size)
{
	int i, j, fileid;

	printf("[BIGFILE] test starting :\n");
	printf("\tCreating a file of size: %d\n", size);

	fileid = open(BIGFILE_NAME, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	if (fileid < 0) {
		err(1, "[BIGFILE]: %s: open for write", BIGFILE_NAME);
	}

	for(i = 0; i < BUFFER_SIZE; i++) {
		fbuffer[i] = LETTER(i);
	}

	printf("\tWriting to file.\n");
	for (i = 0; i < size; i += BUFFER_SIZE) {
		write(fileid, fbuffer, BUFFER_SIZE);

		if (!(i % (10 * BUFFER_SIZE))) {
			printf("\rBW : %d", i);
		}
	}

	printf("\n\tReading from file.\n");
	close(fileid);

	fileid = open(BIGFILE_NAME, O_RDONLY);
	if (fileid < 0) {
		err(1, "[BIGFILE]: %s: open for read", BIGFILE_NAME);
	}

	for (i = 0; i < size; i += BUFFER_SIZE) {
		j = read(fileid, fbuffer, BUFFER_SIZE);
		if (j<0) {
			err(1, "[BIGFILE]: read");
		}
		if (j != BUFFER_SIZE) {
			errx(1, "[BIGFILE]: read: only %d bytes", j);
		}
	}

	if (!(i % (10 * BUFFER_SIZE))) {
		printf("\rBR : %d", i);
	}

	/* Check to see that the data is consistent : */
	for (j = 0; j < BUFFER_SIZE; j++) {
		if (fbuffer[j] != LETTER(j)) {
			errx(1, "[BIGFILE] : Failed read check : "
			     "inconsistent data read: %d", i+j);
		}
	}


	close(fileid);
	if (remove(BIGFILE_NAME)) {
		err(1, "[BIGFILE]: %s: remove", BIGFILE_NAME);
	}

	printf("\n[BIGFILE] : Success!\n");
}

/* ===================================================

 */

static
void
concur(void)
{
	int i, fd;
	int r1, r2, w1;

	printf("Spawning 2 readers, 1 writer.\n");


	fd = open(FNAME, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	if (fd < 0) {
		err(1, "[CONCUR]: %s: open", FNAME);
	}

	printf("Initializing test file: ");

	for (i = 0; i < SECTOR_SIZE + 1; i++) {
		cbuffer[i] = READCHAR;
	}

	for (i = 0; i < TMULT; i++) {
		write(fd, cbuffer, SECTOR_SIZE + 1);
	}


	close(fd);

	printf("Done initializing. Starting processes...\n");

	r1 = forkoff(subproc_read);
	w1 = forkoff(subproc_write);
	r2 = forkoff(subproc_read);

	printf("Waiting for processes.\n");

	dowait(r1);
	dowait(r2);
	dowait(w1);

	if (remove(FNAME)) {
		err(1, "[CONCUR]: %s: remove", FNAME);
	}

	printf("[CONCUR] Done!\n");
}

/* ===================================================

 */

static
void
dir_test(int depth)
{
	int i, fd;
	char tmp[] = DIR_NAME;
	char fmp[] = DIRFILE_NAME;
	char dirname[64];

	strcpy(dirname, ".");

	for (i = 0; i < depth; i++) {
		strcat(dirname, tmp);

		printf("\tCreating dir : %s\n", dirname);

		if (mkdir(dirname, 0775) < 0) {
			err(1, "[DIRTEST]: %s: mkdir", dirname);
		}

		strcat(dirname, fmp);
		printf("\tCreating file: %s\n", dirname);

		fd = open(dirname, O_WRONLY|O_CREAT|O_TRUNC, 0664);
		if (fd<0) {
			err(1, "[DIRTEST]: %s: open", dirname);
		}

		dirname[strlen(dirname) - strlen(fmp)] = '\0';
	}

	printf("[DIRTEST] : Passed directory creation test.\n");

	for (i = 0; i < depth; i++) {
		strcat(dirname, fmp);

		printf("\tDeleting file: %s\n", dirname);

		if (remove(dirname)) {
			 err(1, "[DIRTEST]: %s: remove", dirname);
		}

		dirname[strlen(dirname) - strlen(fmp)] = '\0';
		printf("\tRemoving dir : %s\n", dirname);

		if (rmdir(dirname)) {
			err(1, "[DIRTEST]: %s: rmdir", dirname);
		}

		dirname[strlen(dirname) - strlen(tmp)] = '\0';
	}

	printf("[DIRTEST] : Passed directory removal test.\n");
	printf("[DIRTEST] : Success!\n");
}

/* ===================================================

 */

#define   RUNBIGFILE  0x1
#define   RUNDIRTEST  0x2
#define   RUNCONCUR   0x4
#define   RUNTHEMALL  (RUNBIGFILE | RUNDIRTEST | RUNCONCUR)

int
main(int argc, char * argv[])
{
	int tv = 0;

	if (argc > 1) {
		if (*argv[1]=='1') {
			tv = RUNBIGFILE;
		}
		else if (*argv[1]=='2') {
			tv = RUNDIRTEST;
		}
		else if (*argv[1]=='3') {
			tv = RUNCONCUR;
		}
	}
	else {
		tv = RUNTHEMALL;
	}

	if (tv & RUNBIGFILE) {
		printf("[BIGFILE] : Run #1\n");
		big_file(BIGFILE_SIZE);
		printf("[BIGFILE] : Run #2\n");
		big_file(BIGFILE_SIZE);
	}

	if (tv & RUNDIRTEST) {
		printf("[DIRTEST] : Run #1\n");
		dir_test(DIR_DEPTH);
		printf("[DIRTEST] : Run #2\n");
		dir_test(DIR_DEPTH);
	}

	if (tv & RUNCONCUR) {
		printf("[CONCUR]\n");
		concur();
	}
	return 0;
}


