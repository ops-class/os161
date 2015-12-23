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

#ifndef _SPL_H_
#define _SPL_H_

#include <cdefs.h>

/* Inlining support - for making sure an out-of-line copy gets built */
#ifndef SPL_INLINE
#define SPL_INLINE INLINE
#endif

/*
 * Machine-independent interface to interrupt enable/disable.
 *
 * "spl" stands for "set priority level", and was originally the name of
 * a VAX assembler instruction.
 *
 * The idea is that one can block less important interrupts while
 * processing them, but still allow more urgent interrupts to interrupt
 * that processing.
 *
 * Ordinarily there would be a whole bunch of defined interrupt
 * priority levels and functions for setting them - spltty(),
 * splbio(), etc., etc. But we don't support interrupt priorities in
 * OS/161, so there are only three:
 *
 *      spl0()       sets IPL to 0, enabling all interrupts.
 *      splhigh()    sets IPL to the highest value, disabling all interrupts.
 *      splx(s)      sets IPL to S, enabling whatever state S represents.
 *
 * All three return the old interrupt state. Thus, these are commonly used
 * as follows:
 *
 *      int s = splhigh();
 *      [ code ]
 *      splx(s);
 *
 * Note that these functions only affect interrupts on the current
 * processor.
 */

SPL_INLINE int spl0(void);
SPL_INLINE int splhigh(void);
int splx(int);

/*
 * Integer interrupt priority levels.
 */
#define IPL_NONE   0
#define IPL_HIGH   1

/*
 * Lower-level functions for explicitly raising and lowering
 * particular interrupt levels. These are used by splx() and by the
 * spinlock code.
 *
 * A previous setting of OLDIPL is cancelled and replaced with NEWIPL.
 *
 * For splraise, NEWIPL > OLDIPL, and for spllower, NEWIPL < OLDIPL.
 */
void splraise(int oldipl, int newipl);
void spllower(int oldipl, int newipl);

////////////////////////////////////////////////////////////

SPL_INLINE
int
spl0(void)
{
	return splx(IPL_NONE);
}

SPL_INLINE
int
splhigh(void)
{
	return splx(IPL_HIGH);
}


#endif /* _SPL_H_ */
