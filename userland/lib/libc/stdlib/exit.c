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

#include <stdlib.h>
#include <unistd.h>

/*
 * C standard function: exit process.
 */

void
exit(int code)
{
	/*
	 * In a more complicated libc, this would call functions registered
	 * with atexit() before calling the syscall to actually exit.
	 */

#ifdef __mips__
	/*
	 * Because gcc knows that _exit doesn't return, if we call it
	 * directly it will drop any code that follows it. This means
	 * that if _exit *does* return, as happens before it's
	 * implemented, undefined and usually weird behavior ensues.
	 *
	 * As a hack (this is quite gross) do the call by hand in an
	 * asm block. Then gcc doesn't know what it is, and won't
	 * optimize the following code out, and we can make sure
	 * that exit() at least really does not return.
	 *
	 * This asm block violates gcc's asm rules by destroying a
	 * register it doesn't declare ($4, which is a0) but this
	 * hopefully doesn't matter as the only local it can lose
	 * track of is "code" and we don't use it afterwards.
	 */
	__asm volatile("jal _exit;"	/* call _exit */
		       "move $4, %0"	/* put code in a0 (delay slot) */
		       :		/* no outputs */
		       : "r" (code));	/* code is an input */
	/*
	 * Ok, exiting doesn't work; see if we can get our process
	 * killed by making an illegal memory access. Use a magic
	 * number address so the symptoms are recognizable and
	 * unlikely to occur by accident otherwise.
	 */
	__asm volatile("li $2, 0xeeeee00f;"	/* load magic addr into v0 */
		       "lw $2, 0($2)"		/* fetch from it */
		       :: );			/* no args */
#else
	_exit(code);
#endif
	/*
	 * We can't return; so if we can't exit, the only other choice
	 * is to loop.
	 */
	while (1) { }
}
