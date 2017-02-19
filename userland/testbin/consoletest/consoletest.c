/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *	may be used to endorse or promote products derived from this software
 *	without specific prior written permission.
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
 * consoletest.c
 *
 * 	Tests whether console can be written to.
 *
 *  This should run correctly when open and write syscalls are correctly
 *  implemented.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <test161/test161.h>

// 23 Mar 2012 : GWA : BUFFER_COUNT must be even.

#define STDOUT 1
#define BUFFER_SIZE 1024

#define NSEC_PER_MSEC 1000000ULL
#define MSEC_PER_SEC 1000ULL

static void *
invalid_addr(int max) {
	return (void *)((0x70000000) - (random() % max));
}

static void
init_random() {
	time_t sec;
	unsigned long ns;
	unsigned long long ms;

	__time(&sec, &ns);
	ms = (unsigned long long)sec * MSEC_PER_SEC;
	ms += (ns / NSEC_PER_MSEC);
	srandom((unsigned long)ms);
}

int
main(int argc, char **argv)
{
	// 23 Mar 2012 : GWA : Assume argument passing is *not* supported.

	(void) argc;
	(void) argv;

	int how_many, rv, i, len;
	char buffer[BUFFER_SIZE];

	init_random();

	len = snsecprintf(BUFFER_SIZE, buffer, SECRET, "Able was i ere i saw elbA", "/testbin/consoletest");

	how_many = (random() % 20) + 5;

	for (i = 0; i < how_many; i++) {
		rv = write(1, invalid_addr(0x1000000), len+1);
		if (rv != -1) {
			tprintf("Error: writing to invalid address!\n");
		} else if (errno != EFAULT) {
			tprintf("Error: Expected EFAULT, got %d\n", errno);
		}
	}

	// Insert a '\0' somewhere in the secured string to thwart kprintf attack.
	how_many = (random() % (len - 10)) + 5;
	for (i = BUFFER_SIZE-1; i > how_many; i--) {
		buffer[i] = buffer[i-1];
	}
	buffer[how_many] = '\0';
	++len;

	write(1, buffer, len);
	write(1, "\n", 1);

	return 0;
}
