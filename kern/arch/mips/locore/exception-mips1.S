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
 * Entry points for exceptions.
 *
 * MIPS-1 (r2000/r3000) style exception handling, with the "rfe"
 * instruction rather than "eret", and the three sets of status bits.
 */


   /*
    * Do not allow the assembler to use $1 (at), because we need to be
    * able to save it.
    */
   .set noat
   .set noreorder

/*
 * UTLB exception handler.
 *
 * This code is copied to address 0x80000000, where the MIPS processor
 * automatically invokes it.
 *
 * To avoid colliding with the other exception code, it must not
 * exceed 128 bytes (32 instructions).
 *
 * This is the special entry point for the fast-path TLB refill for
 * faults in the user address space. We don't implement fast-path TLB
 * refill by default. Note that if you do, you either need to make
 * sure the refill code doesn't fault or write extra code in
 * common_exception to tidy up after such faults.
 */

   .text
   .globl mips_utlb_handler
   .type mips_utlb_handler,@function
   .ent mips_utlb_handler
mips_utlb_handler:
   j common_exception		/* Don't need to do anything special */
   nop				/* Delay slot */
   .globl mips_utlb_end
mips_utlb_end:
   .end mips_utlb_handler

/*
 * General exception handler.
 *
 * This code is copied to address 0x80000080, where
 * the MIPS processor automatically invokes it.
 */

   .text
   .globl mips_general_handler
   .type mips_general_handler,@function
   .ent mips_general_handler
mips_general_handler:
   j common_exception		/* Don't need to do anything special */
   nop				/* Delay slot */
   .globl mips_general_end
mips_general_end:
   .end mips_general_handler

   /* This keeps gdb from conflating common_exception and mips_general_end */
   nop				/* padding */


/*
 * Shared exception code for both handlers.
 */

   .text
   .type common_exception,@function
   .ent common_exception
common_exception:
   mfc0 k0, c0_status		/* Get status register */
   andi k0, k0, CST_KUp		/* Check the we-were-in-user-mode bit */
   beq	k0, $0, 1f		/* If clear, from kernel, already have stack */
   nop				/* delay slot */

   /* Coming from user mode - find kernel stack */
   mfc0 k1, c0_context		/* we keep the CPU number here */
   srl k1, k1, CTX_PTBASESHIFT	/* shift it to get just the CPU number */
   sll k1, k1, 2		/* shift it back to make an array index */
   lui k0, %hi(cpustacks)	/* get base address of cpustacks[] */
   addu k0, k0, k1		/* index it */
   move k1, sp			/* Save previous stack pointer in k1 */
   b 2f				/* Skip to common code */
   lw sp, %lo(cpustacks)(k0)	/* Load kernel stack pointer (in delay slot) */
1:
   /* Coming from kernel mode - just save previous stuff */
   move k1, sp			/* Save previous stack in k1 (delay slot) */
2:
   /*
    * At this point:
    *      Interrupts are off. (The processor did this for us.)
    *      k0 contains the value for curthread, to go into s7.
    *      k1 contains the old stack pointer.
    *      sp points into the kernel stack.
    *      All other registers are untouched.
    */

   /*
    * Allocate stack space for 37 words to hold the trap frame,
    * plus four more words for a minimal argument block, plus
    * one more for proper (64-bit) stack alignment.
    */
   addi sp, sp, -168

   /*
    * Save general registers.
    * We exclude k0/k1, which the kernel is free to clobber (and which
    * we already have clobbered), and $0, whose value is fixed.
    *
    * The order here must match mips/include/trapframe.h.
    *
    * gdb disassembles this code to try to figure out what registers
    * are where, and it isn't very bright. So in order to make gdb be
    * able to trace the stack back through here, we play some silly
    * games.
    *
    * In particular:
    *    (1) We store the return address register into the epc slot,
    *        which makes gdb think it's the return address slot. Then
    *        we store the real epc value over that.
    *    (2) We store the current sp into the sp slot, which makes gdb
    *        think it's the stack pointer slot. Then we store the real
    *        value.
    *    (3) gdb also assumes that saved registers in a function are
    *        saved in order. This is why we put epc where it is, and
    *        handle the real value of ra afterwards.
    *    (4) Because gdb will think we're saving k0 and k1, we need to
    *        leave slots for them in the trap frame, even though the
    *        stuff we save there is useless.
    *
    * This logic has not been tested against a recent gdb and has
    * probably bitrotted. Someone(TM) should figure out what gdb
    * currently expects -- or maybe even patch gdb to understand a
    * better form of this that doesn't waste so many cycles.
    */
   sw ra, 160(sp)	/* dummy for gdb */
   sw s8, 156(sp)	/* save s8 */
   sw sp, 152(sp)	/* dummy for gdb */
   sw gp, 148(sp)	/* save gp */
   sw k1, 144(sp)	/* dummy for gdb */
   sw k0, 140(sp)	/* dummy for gdb */

   sw k1, 152(sp)	/* real saved sp */
   nop			/* delay slot for store */

   mfc0 k1, c0_epc	/* Copr.0 reg 13 == PC for exception */
   sw k1, 160(sp)	/* real saved PC */

   sw t9, 136(sp)
   sw t8, 132(sp)
   sw s7, 128(sp)
   sw s6, 124(sp)
   sw s5, 120(sp)
   sw s4, 116(sp)
   sw s3, 112(sp)
   sw s2, 108(sp)
   sw s1, 104(sp)
   sw s0, 100(sp)
   sw t7, 96(sp)
   sw t6, 92(sp)
   sw t5, 88(sp)
   sw t4, 84(sp)
   sw t3, 80(sp)
   sw t2, 76(sp)
   sw t1, 72(sp)
   sw t0, 68(sp)
   sw a3, 64(sp)
   sw a2, 60(sp)
   sw a1, 56(sp)
   sw a0, 52(sp)
   sw v1, 48(sp)
   sw v0, 44(sp)
   sw AT, 40(sp)

   sw ra, 36(sp)

   /*
    * Save special registers.
    */
   mfhi t0
   mflo t1
   sw t0, 32(sp)
   sw t1, 28(sp)

   /*
    * Save remaining exception context information.
    */

   mfc0 t2, c0_status            /* Copr.0 reg 11 == status */
   sw   t2, 20(sp)
   mfc0 t3, c0_vaddr             /* Copr.0 reg 8 == faulting vaddr */
   sw   t3, 16(sp)
   mfc0 t4, c0_cause
   sw   t4, 24(sp)               /* Copr.0 reg 13 == exception cause */

   /*
    * Pretend to save $0 for gdb's benefit.
    */
   sw $0, 12(sp)

   /*
    * Load the curthread register if coming from user mode.
    */
   andi k0, t2, CST_KUp		/* Check the we-were-in-user-mode bit */
   beq	k0, $0, 3f		/* If clear, were in kernel, skip ahead */
   nop				/* delay slot */

   mfc0 k1, c0_context		/* we keep the CPU number here */
   srl k1, k1, CTX_PTBASESHIFT	/* shift it to get just the CPU number */
   sll k1, k1, 2		/* shift it back to make an array index */
   lui k0, %hi(cputhreads)	/* get base address of cputhreads[] */
   addu k0, k0, k1		/* index it */
   lw s7, %lo(cputhreads)(k0)	/* Load curthread value */
3:

   /*
    * Load the kernel GP value.
    */
   la gp, _gp

   /*
    * Prepare to call mips_trap(struct trapframe *)
    */

   addiu a0, sp, 16             /* set argument - pointer to the trapframe */
   jal mips_trap		/* call it */
   nop				/* delay slot */

   /* Something must be here or gdb doesn't find the stack frame. */
   nop

   /*
    * Now restore stuff and return from the exception.
    * Interrupts should be off.
    */
exception_return:

   /*     16(sp)		   no need to restore tf_vaddr */
   lw t0, 20(sp)		/* load status register value into t0 */
   nop				/* load delay slot */
   mtc0 t0, c0_status		/* store it back to coprocessor 0 */
   /*     24(sp)		   no need to restore tf_cause */

   /* restore special registers */
   lw t1, 28(sp)
   lw t0, 32(sp)
   mtlo t1
   mthi t0

   /* load the general registers */
   lw ra, 36(sp)

   lw AT, 40(sp)
   lw v0, 44(sp)
   lw v1, 48(sp)
   lw a0, 52(sp)
   lw a1, 56(sp)
   lw a2, 60(sp)
   lw a3, 64(sp)
   lw t0, 68(sp)
   lw t1, 72(sp)
   lw t2, 76(sp)
   lw t3, 80(sp)
   lw t4, 84(sp)
   lw t5, 88(sp)
   lw t6, 92(sp)
   lw t7, 96(sp)
   lw s0, 100(sp)
   lw s1, 104(sp)
   lw s2, 108(sp)
   lw s3, 112(sp)
   lw s4, 116(sp)
   lw s5, 120(sp)
   lw s6, 124(sp)
   lw s7, 128(sp)
   lw t8, 132(sp)
   lw t9, 136(sp)

   /*     140(sp)		   "saved" k0 was dummy garbage anyway */
   /*     144(sp)		   "saved" k1 was dummy garbage anyway */

   lw gp, 148(sp)		/* restore gp */
   /*     152(sp)		   stack pointer - below */
   lw s8, 156(sp)		/* restore s8 */
   lw k0, 160(sp)		/* fetch exception return PC into k0 */

   lw sp, 152(sp)		/* fetch saved sp (must be last) */

   /* done */
   jr k0			/* jump back */
   rfe				/* in delay slot */
   .end common_exception

/*
 * Code to enter user mode for the first time.
 * Does not return.
 *
 * This is called from mips_usermode().
 * Interrupts on this processor should be off.
 */

   .text
   .globl asm_usermode
   .type asm_usermode,@function
   .ent asm_usermode
asm_usermode:
   /*
    * a0 is the address of a trapframe to use for exception "return".
    * It's allocated on our stack.
    *
    * Move it to the stack pointer - we don't need the actual stack
    * position any more. (When we come back from usermode, cpustacks[]
    * will be used to reinitialize our stack pointer, and that was
    * set by mips_usermode.)
    *
    * Then just jump to the exception return code above.
    */

   j exception_return
   addiu sp, a0, -16		/* in delay slot */
   .end asm_usermode
