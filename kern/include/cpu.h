/*
 * Copyright (c) 2009
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

#ifndef _CPU_H_
#define _CPU_H_


#include <spinlock.h>
#include <threadlist.h>
#include <machine/vm.h>  /* for TLBSHOOTDOWN_MAX */

extern unsigned num_cpus;

/*
 * Per-cpu structure
 *
 * Note: curcpu is defined by <current.h>.
 *
 * cpu->c_self should always be used when *using* the address of curcpu
 * (as opposed to merely dereferencing it) in case curcpu is defined as
 * a pointer with a fixed address and a per-cpu mapping in the MMU.
 */

struct cpu {
	/*
	 * Fixed after allocation.
	 */
	struct cpu *c_self;		/* Canonical address of this struct */
	unsigned c_number;		/* This cpu's cpu number */
	unsigned c_hardware_number;	/* Hardware-defined cpu number */

	/*
	 * Accessed only by this cpu.
	 */
	struct thread *c_curthread;	/* Current thread on cpu */
	struct threadlist c_zombies;	/* List of exited threads */
	unsigned c_hardclocks;		/* Counter of hardclock() calls */
	unsigned c_spinlocks;		/* Counter of spinlocks held */

	/*
	 * Accessed by other cpus.
	 * Protected by the runqueue lock.
	 */
	bool c_isidle;			/* True if this cpu is idle */
	struct threadlist c_runqueue;	/* Run queue for this cpu */
	struct spinlock c_runqueue_lock;

	/*
	 * Accessed by other cpus.
	 * Protected by the IPI lock.
	 *
	 * TLB shootdown requests made to this CPU are queued in
	 * c_shootdown[], with c_numshootdown holding the number of
	 * requests. TLBSHOOTDOWN_MAX is the maximum number that can
	 * be queued at once, which is machine-dependent.
	 *
	 * The contents of struct tlbshootdown are also machine-
	 * dependent and might reasonably be either an address space
	 * and vaddr pair, or a paddr, or something else.
	 */
	uint32_t c_ipi_pending;		/* One bit for each IPI number */
	struct tlbshootdown c_shootdown[TLBSHOOTDOWN_MAX];
	unsigned c_numshootdown;
	struct spinlock c_ipi_lock;

	/*
	 * Accessed by other cpus. Protected inside hangman.c.
	 */
	HANGMAN_ACTOR(c_hangman);
};

/*
 * Initialization functions.
 *
 * cpu_create creates a cpu; it is suitable for calling from driver-
 * or bus-specific code that looks for secondary CPUs.
 *
 * cpu_create calls cpu_machdep_init.
 *
 * cpu_start_secondary is the platform-dependent assembly language
 * entry point for new CPUs; it can be found in start.S. It calls
 * cpu_hatch after having claimed the startup stack and thread created
 * for the cpu.
 */
struct cpu *cpu_create(unsigned hardware_number);
void cpu_machdep_init(struct cpu *);
/*ASMLINKAGE*/ void cpu_start_secondary(void);
void cpu_hatch(unsigned software_number);

/*
 * Produce a string describing the CPU type.
 */
void cpu_identify(char *buf, size_t max);

/*
 * Hardware-level interrupt on/off, for the current CPU.
 *
 * These should only be used by the spl code.
 */
void cpu_irqoff(void);
void cpu_irqon(void);

/*
 * Idle or shut down (respectively) the processor.
 *
 * cpu_idle() sits around (in a low-power state if possible) until it
 * thinks something interesting may have happened, such as an
 * interrupt. Then it returns. (It may be wrong, so it should always
 * be called in a loop checking some other condition.) It must be
 * called with interrupts off to avoid race conditions, although
 * interrupts may be delivered before it returns.
 *
 * cpu_halt sits around (in a low-power state if possible) until the
 * external reset is pushed. Interrupts should be disabled. It does
 * not return. It should not allow interrupts to be delivered.
 */
void cpu_idle(void);
void cpu_halt(void);

/*
 * Interprocessor interrupts.
 *
 * From time to time it is necessary to poke another CPU. System
 * boards of multiprocessor machines provide a way to do this.
 *
 * TLB shootdown is done by the VM system when more than one processor
 * has (or may have) a page mapped in the MMU and it is being changed
 * or otherwise needs to be invalidated across all CPUs.
 *
 * ipi_send sends an IPI to one CPU.
 * ipi_broadcast sends an IPI to all CPUs except the current one.
 * ipi_tlbshootdown is like ipi_send but carries TLB shootdown data.
 *
 * interprocessor_interrupt is called on the target CPU when an IPI is
 * received.
 */

/* IPI types */
#define IPI_PANIC		0	/* System has called panic() */
#define IPI_OFFLINE		1	/* CPU is requested to go offline */
#define IPI_UNIDLE		2	/* Runnable threads are available */
#define IPI_TLBSHOOTDOWN	3	/* MMU mapping(s) need invalidation */

void ipi_send(struct cpu *target, int code);
void ipi_broadcast(int code);
void ipi_tlbshootdown(struct cpu *target, const struct tlbshootdown *mapping);

void interprocessor_interrupt(void);


#endif /* _CPU_H_ */
