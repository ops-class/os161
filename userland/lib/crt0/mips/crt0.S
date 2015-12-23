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
 * crt0.o for MIPS r2000/r3000.
 *
 * crt stands for "C runtime".
 *
 * Basically, this is the startup code that gets invoked before main(),
 * and regains control when main returns.
 *
 * All we really do is save copies of argv and environ for use by libc
 * funcions (e.g. err* and warn*), and call exit when main returns.
 */

#include <kern/mips/regdefs.h>
#include <kern/syscall.h>

	.set noreorder	/* so we can use delay slots explicitly */

	.text
	.globl __start
	.type __start,@function
	.ent __start
__start:
	/* Load the "global pointer" register */
	la gp, _gp

   	/*
	 * We expect that the kernel passes argc in a0, argv in a1,
	 * and environ in a2. We do not expect the kernel to set up a
	 * complete stack frame, however.
	 *
	 * The MIPS ABI decrees that every caller will leave 16 bytes of
	 * space in the bottom of its stack frame for writing back the
	 * values of a0-a3, even when calling functions that take fewer
	 * than four arguments. It also requires the stack to be aligned
	 * to an 8-byte boundary. (This is because of 64-bit MIPS, which
	 * we're not dealing with... but we'll conform to the standard.)
	 */
	li t0, 0xfffffff8		/* mask for stack alignment */
	and sp, sp, t0			/* align the stack */
	addiu sp, sp, -16		/* create our frame */

	sw a1, __argv	/* save second arg (argv) in __argv for use later */
	sw a2, __environ /* save third arg (environ) for use later */

	jal main	/* call main */
	nop		/* delay slot */

	/*
	 * Now, we have the return value of main in v0.
	 *
	 * Move it to s0 (which is callee-save) so we still have
	 * it in case exit() returns.
	 *
	 * Also move it to a0 so it's the argument to exit.
	 */
	move s0, v0	/* save return value */
1:
	jal exit	/* call exit() */
	move a0, s0   	/* Set argument (in delay slot) */

	/*
   	 * exit() does not return. (Even if _exit() is not implemented
	 * so *it* returns, exit() is supposed to take care of that.)
	 * But just in case, loop and try again.
	 */
	b 1b		/* loop back */
	nop		/* delay slot */
	.end __start
