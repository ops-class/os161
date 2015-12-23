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
 * Assembler-level thread startup trampoline.
 */

#include <kern/mips/regdefs.h>

   .text
   .set noreorder

   .globl mips_threadstart
   .type mips_threadstart,@function
   .ent mips_threadstart
mips_threadstart:

   /*
    * This code doesn't take normal arguments. It's reached when
    * switchframe_switch switches to a new thread. switchframe_switch
    * does "j ra"; ra gets preloaded in switchframe_init to come here.
    *
    * Our arguments are in callee-save registers, as follows:
    *
    *   s0   entrypoint
    *   s1   data1
    *   s2   data2
    *
    * We need to rearrange these so as to call the normal C function
    * thread_startup(void (*entrypoint)(), void *data1, unsigned long data2).
    */

   addiu sp, sp, -16    /* make our stack frame */

   move ra, $0          /* clear return addr so we're top of the call stack */

   move a0, s0          /* load arguments and call */
   move a1, s1
   j thread_startup
   move a2, s2          /* (in delay slot) */

   .end mips_threadstart
