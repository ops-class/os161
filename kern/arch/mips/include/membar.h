/*
 * Copyright (c) 2013
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

#ifndef _MIPS_MEMBAR_H_
#define _MIPS_MEMBAR_H_

/*
 * On the mips there's only one memory barrier instruction, so these
 * are all the same. This is not true on many other CPUs (x86, arm,
 * sparc, powerpc, etc.) We also mark the instruction as a compiler-
 * level barrier by telling gcc that it destroys memory; this prevents
 * gcc from reordering loads and stores around it.
 *
 * See include/membar.h for further information.
 */

MEMBAR_INLINE
void
membar_any_any(void)
{
	__asm volatile(
		".set push;"		/* save assembler mode */
		".set mips32;"		/* allow MIPS32 instructions */
		"sync;"			/* do it */
		".set pop"		/* restore assembler mode */
		:			/* no outputs */
		:			/* no inputs */
		: "memory");		/* "changes" memory */
}

MEMBAR_INLINE void membar_load_load(void) { membar_any_any(); }
MEMBAR_INLINE void membar_store_store(void) { membar_any_any(); }
MEMBAR_INLINE void membar_store_any(void) { membar_any_any(); }
MEMBAR_INLINE void membar_any_store(void) { membar_any_any(); }


#endif /* _MIPS_MEMBAR_H_ */
