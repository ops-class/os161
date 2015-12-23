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
 * CPU control functions.
 */

#include <types.h>
#include <lib.h>
#include <mips/specialreg.h>
#include <mips/trapframe.h>
#include <platform/maxcpus.h>
#include <cpu.h>
#include <thread.h>

////////////////////////////////////////////////////////////

/*
 * Startup and exception-time stack hook.
 *
 * The MIPS lacks a good way to find the current CPU, current thread,
 * or current thread stack upon trap entry from user mode. To deal
 * with this, we store the CPU number (our number, not the hardware
 * number) in a nonessential field in the MMU, which is about the only
 * place possible, and then use that to index cpustacks[]. This gets
 * us the value to load as the stack pointer. We can then also load
 * curthread from cputhreads[] by parallel indexing.
 *
 * These arrays are also used to start up new CPUs, for roughly the
 * same reasons.
 */

vaddr_t cpustacks[MAXCPUS];
vaddr_t cputhreads[MAXCPUS];

/*
 * Do machine-dependent initialization of the cpu structure or things
 * associated with a new cpu. Note that we're not running on the new
 * cpu when this is called.
 */
void
cpu_machdep_init(struct cpu *c)
{
	vaddr_t stackpointer;

	KASSERT(c->c_number < MAXCPUS);

	if (c->c_curthread->t_stack == NULL) {
		/* boot cpu; don't need to do anything here */
	}
	else {
		/*
		 * Stick the stack in cpustacks[], and thread pointer
		 * in cputhreads[].
		 */

		/* stack base address */
		stackpointer = (vaddr_t) c->c_curthread->t_stack;
		/* since stacks grow down, get the top */
		stackpointer += STACK_SIZE;

		cpustacks[c->c_number] = stackpointer;
		cputhreads[c->c_number] = (vaddr_t)c->c_curthread;
	}
}

////////////////////////////////////////////////////////////

/*
 * Return the type name of the currently running CPU.
 *
 * For now, assume we're running on System/161 so we can use the
 * System/161 processor-ID values.
 */

#define SYS161_PRID_ORIG	0x000003ff
#define SYS161_PRID_2X		0x000000a1

static inline
uint32_t
cpu_getprid(void)
{
	uint32_t prid;

	__asm volatile("mfc0 %0,$15" : "=r" (prid));
	return prid;
}

static inline
uint32_t
cpu_getfeatures(void)
{
	uint32_t features;

	__asm volatile(".set push;"		/* save assembler mode */
		       ".set mips32;"		/* allow mips32 instructions */
		       "mfc0 %0,$15,1;"		/* get cop0 reg 15 sel 1 */
		       ".set pop"		/* restore assembler mode */
		       : "=r" (features));
	return features;
}

static inline
uint32_t
cpu_getifeatures(void)
{
	uint32_t features;

	__asm volatile(".set push;"		/* save assembler mode */
		       ".set mips32;"		/* allow mips32 instructions */
		       "mfc0 %0,$15,2;"		/* get cop0 reg 15 sel 2 */
		       ".set pop"		/* restore assembler mode */
		       : "=r" (features));
	return features;
}

void
cpu_identify(char *buf, size_t max)
{
	uint32_t prid;
	uint32_t features;

	prid = cpu_getprid();
	switch (prid) {
	    case SYS161_PRID_ORIG:
		snprintf(buf, max, "MIPS/161 (System/161 1.x and pre-2.x)");
		break;
	    case SYS161_PRID_2X:
		features = cpu_getfeatures();
		snprintf(buf, max, "MIPS/161 (System/161 2.x) features 0x%x",
			 features);
		features = cpu_getifeatures();
		if (features != 0) {
			kprintf("WARNING: unknown CPU incompatible features "
				"0x%x\n", features);
		}
		break;
	    default:
		snprintf(buf, max, "32-bit MIPS (unknown type, CPU ID 0x%x)",
			 prid);
		break;
	}
}

////////////////////////////////////////////////////////////

/*
 * Interrupt control.
 *
 * While the mips actually has on-chip interrupt priority masking, in
 * the interests of simplicity, we don't use it. Instead we use
 * coprocessor 0 register 12 (the system coprocessor "status"
 * register) bit 0, IEc, which is the global interrupt enable flag.
 * (IEc stands for interrupt-enable-current.)
 */

/*
 * gcc inline assembly to get at the status register.
 *
 * Pipeline hazards:
 *    - there must be at least one cycle between GET_STATUS
 *      and SET_STATUS;
 *    - it may take up to three cycles after SET_STATUS for the
 *      interrupt state to really change.
 *
 * These considerations do not (currently) apply to System/161,
 * however.
 */
#define GET_STATUS(x) __asm volatile("mfc0 %0,$12" : "=r" (x))
#define SET_STATUS(x) __asm volatile("mtc0 %0,$12" :: "r" (x))

/*
 * Interrupts on.
 */
void
cpu_irqon(void)
{
        uint32_t x;

        GET_STATUS(x);
        x |= CST_IEc;
        SET_STATUS(x);
}

/*
 * Interrupts off.
 */
void
cpu_irqoff(void)
{
        uint32_t x;

        GET_STATUS(x);
        x &= ~(uint32_t)CST_IEc;
        SET_STATUS(x);
}

/*
 * Used below.
 */
static
void
cpu_irqonoff(void)
{
        uint32_t x, xon, xoff;

        GET_STATUS(x);
        xon = x | CST_IEc;
        xoff = x & ~(uint32_t)CST_IEc;
        SET_STATUS(xon);
	__asm volatile("nop; nop; nop; nop");
        SET_STATUS(xoff);
}

////////////////////////////////////////////////////////////

/*
 * Idling.
 */

/*
 * gcc inline assembly for the WAIT instruction.
 *
 * mips r2k/r3k has no idle instruction at all.
 *
 * However, to avoid completely overloading the computing cluster, we
 * appropriate the mips32 WAIT instruction.
 */

static
inline
void
wait(void)
{
	/*
	 * The WAIT instruction goes into powersave mode until an
	 * interrupt is trying to occur.
	 *
	 * Then switch interrupts on and off again, so we actually
	 * take the interrupt.
	 *
	 * Note that the precise behavior of this instruction in the
	 * System/161 simulator is partly guesswork. This code may not
	 * work on a real mips.
	 */
	__asm volatile(
		".set push;"		/* save assembler mode */
		".set mips32;"		/* allow MIPS32 instructions */
		".set volatile;"	/* avoid unwanted optimization */
		"wait;"			/* suspend until interrupted */
		".set pop"		/* restore assembler mode */
	      );
}

/*
 * Idle the processor until something happens.
 */
void
cpu_idle(void)
{
	wait();
        cpu_irqonoff();
}

/*
 * Halt the CPU permanently.
 */
void
cpu_halt(void)
{
        cpu_irqoff();
        while (1) {
		wait();
        }
}
