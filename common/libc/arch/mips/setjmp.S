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
 * setjmp and longjmp for MIPS.
 */

#include <kern/mips/regdefs.h>

   .text
   .set noreorder

   /*
    * int setjmp(jmp_buf jb);
    *
    * Save the current state so we can return again from the call later
    * if/when longjmp is called. (If the function that called setjmp
    * returns before longjmp is called, the results are undefined. We
    * only need to save registers, not the whole contents of the stack.)
    */

   .globl setjmp
   .type setjmp,@function
   .ent setjmp
setjmp:
   /*
    * jmp_buf is in a0. We need to save s0-s8, sp, and ra in it.
    * Don't store more registers without adjusting machine/setjmp.h.
    */

   sw sp, 0(a0)		/* save registers */
   sw ra, 4(a0)
   sw s0, 8(a0)
   sw s1, 12(a0)
   sw s2, 16(a0)
   sw s3, 20(a0)
   sw s4, 24(a0)
   sw s5, 28(a0)
   sw s6, 32(a0)
   sw s7, 36(a0)
   sw s8, 40(a0)

   j ra			/* done */
   li v0, 0		/* return 0 (in delay slot) */
   .end setjmp


   /*
    * void longjmp(jmp_buf jb, int code);
    */
   .globl longjmp
   .type longjmp,@function
   .ent longjmp
longjmp:
   /*
    * jmp_buf is in a0. Return code is in a1.
    * We need to restore s0-s8, sp, and ra from the jmp_buf.
    * The return code is forced to 1 if 0 is passed in.
    */

   sltiu t0, a1, 1	/* set t0 to 1 if return code is 0... otherwise 0 */
   addu a1, a1, t0	/* update the return code */

   lw sp, 0(a0)		/* restore registers */
   lw ra, 4(a0)
   lw s0, 8(a0)
   lw s1, 12(a0)
   lw s2, 16(a0)
   lw s3, 20(a0)
   lw s4, 24(a0)
   lw s5, 28(a0)
   lw s6, 32(a0)
   lw s7, 36(a0)
   lw s8, 40(a0)

   j ra			/* return, to where setjmp was called from */
   move v0, a1		/* set return value */
   .end longjmp
