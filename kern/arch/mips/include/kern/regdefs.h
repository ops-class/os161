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
 * Macros for general-purpose register numbers for MIPS.
 *
 * Exported to userlevel because it's ~standard for that architecture.
 */

#ifndef _KERN_MIPS_REGDEFS_H_
#define _KERN_MIPS_REGDEFS_H_


#define z0  $0     /* always zero register */
#define AT  $1     /* assembler temp register */
#define v0  $2     /* value 0 */
#define v1  $3     /* value 1 */
#define a0  $4     /* argument 0 */
#define a1  $5     /* argument 1 */
#define a2  $6     /* argument 2 */
#define a3  $7     /* argument 3 */
#define t0  $8     /* temporary (caller-save) 0 */
#define t1  $9     /* temporary (caller-save) 1 */
#define t2  $10    /* temporary (caller-save) 2 */
#define t3  $11    /* temporary (caller-save) 3 */
#define t4  $12    /* temporary (caller-save) 4 */
#define t5  $13    /* temporary (caller-save) 5 */
#define t6  $14    /* temporary (caller-save) 6 */
#define t7  $15    /* temporary (caller-save) 7 */
#define s0  $16    /* saved (callee-save) 0 */
#define s1  $17    /* saved (callee-save) 1 */
#define s2  $18    /* saved (callee-save) 2 */
#define s3  $19    /* saved (callee-save) 3 */
#define s4  $20    /* saved (callee-save) 4 */
#define s5  $21    /* saved (callee-save) 5 */
#define s6  $22    /* saved (callee-save) 6 */
#define s7  $23    /* saved (callee-save) 7 */
#define t8  $24    /* temporary (caller-save) 8 */
#define t9  $25    /* temporary (caller-save) 9 */
#define k0  $26    /* kernel temporary 0 */
#define k1  $27    /* kernel temporary 1 */
#define gp  $28    /* global pointer */
#define sp  $29    /* stack pointer */
#define s8  $30    /* saved (callee-save) 8 = frame pointer */
#define ra  $31    /* return address */


#endif /* _KERN_MIPS_REGDEFS_H_ */
