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

#include <kern/mips/regdefs.h>
#include <mips/specialreg.h>

/*
 * TLB handling for the MIPS-161.
 *
 * The MIPS-161 uses the simpler MIPS-1 (r2000/r3000) TLB rather
 * than the paired-page TLB of later MIPS models.
 *
 * However, we handle MIPS32 pipeline hazards. If you want to run on
 * a real MIPS-1, change the ssnops to plain nops and check where and
 * how many you need in the matching processor docs.
 *
 * (ssnop means "superscalar nop"; it exists because the pipeline
 * hazards require a fixed number of cycles, and a superscalar CPU can
 * potentially issue arbitrarily many nops in one cycle.)
 */

   .text
   .set noreorder
   .set mips32 /* so we can use ssnop */

   /*
    * tlb_random: use the "tlbwr" instruction to write a TLB entry
    * into a (very pseudo-) random slot in the TLB.
    *
    * Pipeline hazard: must wait between setting entryhi/lo and
    * doing the tlbwr. Use two cycles; some processors may vary.
    */
   .globl tlb_random
   .type tlb_random,@function
   .ent tlb_random
tlb_random:
   mtc0 a0, c0_entryhi	/* store the passed entry into the */
   mtc0 a1, c0_entrylo	/*   tlb entry registers */
   ssnop		/* wait for pipeline hazard */
   ssnop
   tlbwr		/* do it */
   j ra
   nop
   .end tlb_random

   /*
    * tlb_write: use the "tlbwi" instruction to write a TLB entry
    * into a selected slot in the TLB.
    *
    * Pipeline hazard: must wait between setting entryhi/lo and
    * doing the tlbwi. Use two cycles; some processors may vary.
    */
   .text
   .globl tlb_write
   .type tlb_write,@function
   .ent tlb_write
tlb_write:
   mtc0 a0, c0_entryhi	/* store the passed entry into the */
   mtc0 a1, c0_entrylo	/*   tlb entry registers */
   sll  t0, a2, CIN_INDEXSHIFT  /* shift the passed index into place */
   mtc0 t0, c0_index	/* store the shifted index into the index register */
   ssnop		/* wait for pipeline hazard */
   ssnop
   tlbwi		/* do it */
   j ra
   nop
   .end tlb_write

   /*
    * tlb_read: use the "tlbr" instruction to read a TLB entry
    * from a selected slot in the TLB.
    *
    * Pipeline hazard: must wait between setting c0_index and
    * doing the tlbr. Use two cycles; some processors may vary.
    * Similarly, three more cycles before reading c0_entryhi/lo.
    */
   .text
   .globl tlb_read
   .type tlb_read,@function
   .ent tlb_read
tlb_read:
   sll  t0, a2, CIN_INDEXSHIFT  /* shift the passed index into place */
   mtc0 t0, c0_index	/* store the shifted index into the index register */
   ssnop		/* wait for pipeline hazard */
   ssnop
   tlbr			/* do it */
   ssnop		/* wait for pipeline hazard */
   ssnop
   ssnop
   mfc0 t0, c0_entryhi	/* get the tlb entry out of the */
   mfc0 t1, c0_entrylo	/*   tlb entry registers */
   sw t0, 0(a0)		/* store through the passed pointer */
   j ra
   sw t1, 0(a1)		/* store (in delay slot) */
   .end tlb_read

   /*
    * tlb_probe: use the "tlbp" instruction to find the index in the
    * TLB of a TLB entry matching the relevant parts of the one supplied.
    *
    * Pipeline hazard: must wait between setting c0_entryhi/lo and
    * doing the tlbp. Use two cycles; some processors may vary.
    * Similarly, two more cycles before reading c0_index.
    */
   .text
   .globl tlb_probe
   .type tlb_probe,@function
   .ent tlb_probe
tlb_probe:
   mtc0 a0, c0_entryhi	/* store the passed entry into the */
   mtc0 a1, c0_entrylo	/*   tlb entry registers */
   ssnop		/* wait for pipeline hazard */
   ssnop
   tlbp			/* do it */
   ssnop		/* wait for pipeline hazard */
   ssnop
   mfc0 t0, c0_index	/* fetch the index back in t0 */

   /*
    * If the high bit (CIN_P) of c0_index is set, the probe failed.
    * The high bit is not set <--> c0_index (now in t0) >= 0.
    */

   bgez t0, 1f		/* did probe succeed? if so, skip forward */
   nop			/* delay slot */
   addi v0, z0, -1	/* set return value to -1 to indicate failure */
   j ra			/* done */
   nop			/* delay slot */

1:
   /* succeeded - get the index field from the index register value */
   andi t1, t0, CIN_INDEX       /* mask off the field */
   j ra				/* done */
   sra  v0, t1, CIN_INDEXSHIFT  /* shift it (in delay slot) */
   .end tlb_probe


   /*
    * tlb_reset
    *
    * Initialize the TLB. At processor startup, the TLB state is completely
    * undefined. So be sure to avoid creating any duplicates. Also make sure
    * that the initialization entries don't duplicate the INVALID entries
    * defined in tlb.h. (This way you can write the invalid entries in
    * without having to use tlbp to find out if they're going to cause dups.)
    *
    * This function is not defined in tlb.h because it's only called from
    * start.S.
    *
    * Pipeline hazards are as above.
    */
   .text
   .globl tlb_reset
   .type tlb_reset,@function
   .ent tlb_reset
tlb_reset:
   li t0, 0			/* t0 <- tlb index number (shifted) */
   li t1, 0x81000000		/* t1 <- tlb reset vaddr */
1:
   mtc0 $0, c0_entrylo 		/* set up proposed tlb entry for reset */
   mtc0 t1, c0_entryhi
   ssnop			/* wait for pipeline hazard */
   ssnop
   tlbp				/* check if it already exists */
   ssnop			/* wait for pipeline hazard */
   ssnop
   mfc0 t2, c0_index
   bgez t2, 1b			/* if it does, loop back */
   addiu t1, t1, 0x1000		/* next vaddr (in delay slot) */
   mtc0 t0, c0_index		/* doesn't exist, set index to write to */
   ssnop			/* wait for pipeline hazard */
   ssnop
   addiu t0, t0, 0x100		/* next tlb index (shifted) */
   bne t0, 0x4000, 1b		/* if it's not the last tlb index, loop */
   tlbwi			/* write tlb entry (in delay slot) */
   j ra				/* done */
   nop				/* delay slot */
   .end tlb_reset
