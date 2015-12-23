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

#ifndef _MIPS_SPECIALREG_H_
#define _MIPS_SPECIALREG_H_


/*
 * Coprocessor 0 (system processor) register numbers
 */
#define c0_index    $0          /* TLB entry index register */
#define c0_random   $1          /* TLB random slot register */
#define c0_entrylo  $2          /* TLB entry contents (low-order half) */
/*      c0_entrylo0 $2 */       /* MIPS-II and up only */
/*      c0_entrylo1 $3 */       /* MIPS-II and up only */
#define c0_context  $4          /* some precomputed pagetable stuff */
/*      c0_pagemask $5 */       /* MIPS-II and up only */
/*      c0_wired    $6 */       /* MIPS-II and up only */
#define c0_vaddr    $8          /* virtual addr of failing memory access */
#define c0_count    $9          /* cycle counter (MIPS-II and up) */
#define c0_entryhi  $10         /* TLB entry contents (high-order half) */
#define c0_compare  $11         /* on-chip timer control (MIPS-II and up) */
#define c0_status   $12         /* processor status register */
#define c0_cause    $13         /* exception cause register */
#define c0_epc      $14         /* exception PC register */
#define c0_prid     $15         /* processor ID register */
/*      c0_config   $16 */      /* MIPS-II and up only */
/*      c0_lladdr   $17 */      /* MIPS-II and up only */
/*      c0_watchlo  $18 */      /* MIPS-II and up only */
/*      c0_watchhi  $19 */      /* MIPS-II and up only */

/*
 * Mode bits in c0_status
 */
#define CST_IEc      0x00000001 /* current: interrupt enable */
#define CST_KUc      0x00000002 /* current: user mode */
#define CST_IEp      0x00000004 /* previous: interrupt enable */
#define CST_KUp      0x00000008 /* previous: user mode */
#define CST_IEo      0x00000010 /* old: interrupt enable */
#define CST_KUo      0x00000020 /* old: user mode */
#define CST_MODEMASK 0x0000003f /* mask for the above */
#define CST_IRQMASK  0x0000ff00 /* mask for the individual irq enable bits */
#define CST_BEV      0x00400000 /* bootstrap exception vectors flag */

/*
 * Fields of the c0_cause register
 */
#define CCA_UTLB   0x00000001   /* true if UTLB exception (set by our asm) */
#define CCA_CODE   0x0000003c   /* EX_foo in trapframe.h */
#define CCA_IRQS   0x0000ff00   /* Currently pending interrupts */
#define CCA_COPN   0x30000000   /* Coprocessor number for EX_CPU */
#define CCA_JD     0x80000000   /* True if exception happened in jump delay */

#define CCA_CODESHIFT   2       /* shift for CCA_CODE field */

/*
 * Fields of the c0_index register
 */
#define CIN_P      0x80000000   /* nonzero -> TLB probe found nothing */
#define CIN_INDEX  0x00003f00   /* 6-bit index into TLB */

#define CIN_INDEXSHIFT  8       /* shift for CIN_INDEX field */

/*
 * Fields of the c0_context register
 *
 * The intent of c0_context is that you can manage virtually-mapped
 * page tables in kseg2; then you load the base address of the current
 * page table into c0_context. On a TLB miss the failing address is
 * masked and shifted and appears in the VSHIFT field, and c0_context
 * thereby contains the address of the page table entry you need to
 * load into the TLB. This can be used to make TLB refill very fast.
 *
 * However, in OS/161 we use CTX_PTBASE to hold the current CPU
 * number. This (or something like it) is fairly important to have and
 * there's no other good place in the chip to put it. See discussions
 * elsewhere.
 */
#define CTX_VSHIFT 0x001ffffc	/* shifted/masked copy of c0_vaddr */
#define CTX_PTBASE 0xffe00000	/* page table base address */

#define CTX_PTBASESHIFT  21	/* shift for CTX_PBASE field */

/*
 * Hardwired exception handler addresses.
 */
#define EXADDR_UTLB	0x80000000
#define EXADDR_GENERAL	0x80000080


#endif /* _MIPS_SPECIALREG_H_ */
