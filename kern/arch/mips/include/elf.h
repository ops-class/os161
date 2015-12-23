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

#ifndef _MIPS_ELF_H_
#define _MIPS_ELF_H_

/*
 * MIPS machine-dependent definitions for the ELF binary format.
 */


/* The ELF executable type. */
#define EM_MACHINE  EM_MIPS

/* Linker relocation codes.	   SIZE   DESCRIPTION */
#define R_MIPS_NONE	0	/* ---    nop */
#define R_MIPS_16	1	/* u16    value */
#define R_MIPS_32	2	/* u32    value */
#define R_MIPS_REL32	3	/* u32    value relative to patched address */
#define R_MIPS_26	4	/* u26    j/jal instruction address field */
#define R_MIPS_HI16	5	/* u16    %hi(sym) as below */
#define R_MIPS_LO16	6	/* s16    %lo(sym) as below */
#define R_MIPS_GPREL16	7	/* s16    offset from GP register */
#define R_MIPS_LITERAL	8	/* s16    GPREL16 for file-local symbols (?) */
#define R_MIPS_GOT16	9	/* u16    offset into global offset table */
#define R_MIPS_PC16	10	/* s16    PC-relative reference */
#define R_MIPS_CALL16	11	/* u16    call through global offset table */
#define R_MIPS_GPREL32	12	/* s32    offset from GP register */
/* %hi/%lo are defined so %hi(sym) << 16 + %lo(sym) = sym */


#endif /* _MIPS_ELF_H_ */
