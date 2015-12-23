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
 * This file is copied to syscalls.S, and then the actual syscalls are
 * appended as lines of the form
 *    SYSCALL(symbol, number)
 *
 * Warning: gccs before 3.0 run cpp in -traditional mode on .S files.
 * So if you use an older gcc you'll need to change the token pasting
 * in SYSCALL().
 */

#include <kern/syscall.h>
#include <machine/regdefs.h>

/*
 * Definition for each syscall.
 * All we do is load the syscall number into v0, the register the
 * kernel expects to find it in, and jump to the shared syscall code.
 * (Note that the addiu instruction is in the jump's delay slot.)
 */
#define SYSCALL(sym, num) \
   .set noreorder		; \
   .globl sym			; \
   .type sym,@function		; \
   .ent sym			; \
sym:				; \
   j __syscall                  ; \
   addiu v0, $0, SYS_##sym	; \
   .end sym			; \
   .set reorder

/*
 * Now, the shared system call code.
 * The MIPS syscall ABI is as follows:
 *
 *    On entry, call number in v0. The rest is like a normal function
 *    call: four args in a0-a3, the other args on the stack.
 *
 *    On successful return, zero in a3 register; return value in v0
 *    (v0 and v1 for a 64-bit return value).
 *
 *    On error return, nonzero in a3 register; errno value in v0.
 *
 * The use of a3 as a return register to hold the success flag is
 * gross, but I didn't make it up.
 *
 * Note that by longstanding Unix convention and POSIX decree, errno
 * is not to be set unless the call actually fails.
 */

   .set noreorder
   .text
   .type __syscall,@function
   .ent __syscall
__syscall:
   syscall              /* make system call */
   beq a3, $0, 1f       /* if a3 is zero, call succeeded */
   nop			/* delay slot */
   sw v0, errno        /* call failed: store errno */
   li v1, -1		/* and force return value to -1 */
   li v0, -1
1:
   j ra			/* return */
   nop			/* delay slot */
   .end __syscall
   .set reorder

