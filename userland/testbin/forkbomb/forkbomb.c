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
 * forkbomb - apply malthus to an operating system ;-)
 *
 * DO NOT RUN THIS ON A REAL SYSTEM - IT WILL GRIND TO A HALT AND
 * PEOPLE WILL COME AFTER YOU WIELDING BASEBALL BATS OR THE AD
 * BOARD(*). WE WARNED YOU.
 *
 * We don't expect your system to withstand this without grinding to
 * a halt, but once your basic system calls are complete it shouldn't
 * crash. Likewise for after your virtual memory system is complete.
 *
 * (...at least in an ideal world. However, it can be difficult to
 * handle all the loose ends involved. Heroic measures are not
 * expected. If in doubt, talk to the course staff.)
 *
 *
 * (*) The Administrative Board of Harvard College handles formal
 * disciplinary action.
 */

#include <unistd.h>
#include <err.h>

static volatile int pid;

int
main(void)
{
	int i;

	while (1) {
		fork();

		pid = getpid();

		/* Make sure each fork has its own address space. */
		for (i=0; i<300; i++) {
			volatile int seenpid;
			seenpid = pid;
			if (seenpid != getpid()) {
				errx(1, "pid mismatch (%d, should be %d) "
				     "- your vm is broken!",
				     seenpid, getpid());
			}
		}
	}
}
