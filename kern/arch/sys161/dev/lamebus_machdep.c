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

#include <types.h>
#include <kern/unistd.h>
#include <lib.h>
#include <mips/specialreg.h>
#include <mips/trapframe.h>
#include <cpu.h>
#include <spl.h>
#include <clock.h>
#include <thread.h>
#include <current.h>
#include <membar.h>
#include <synch.h>
#include <mainbus.h>
#include <sys161/bus.h>
#include <lamebus/lamebus.h>
#include <lamebus/ltrace.h>
#include "autoconf.h"

/*
 * CPU frequency used by the on-chip timer.
 *
 * Note that we really ought to measure the CPU frequency against the
 * real-time clock instead of compiling it in like this.
 */
#define CPU_FREQUENCY 25000000 /* 25 MHz */

/*
 * Access to the on-chip timer.
 *
 * The c0_count register increments on every cycle; when the value
 * matches the c0_compare register, the timer interrupt line is
 * asserted. Writing to c0_compare again clears the interrupt.
 */
static
void
mips_timer_set(uint32_t count)
{
	/*
	 * $11 == c0_compare; we can't use the symbolic name inside
	 * the asm string.
	 */
	__asm volatile(
		".set push;"		/* save assembler mode */
		".set mips32;"		/* allow MIPS32 registers */
		"mtc0 %0, $11;"		/* do it */
		".set pop"		/* restore assembler mode */
		:: "r" (count));
}

/*
 * LAMEbus data for the system. (We have only one LAMEbus per system.)
 * This does not need to be locked, because it's constant once
 * initialized, and initialized before we start other threads or CPUs.
 */
static struct lamebus_softc *lamebus;

void
mainbus_bootstrap(void)
{
	/* Interrupts should be off (and have been off since startup) */
	KASSERT(curthread->t_curspl > 0);

	/* Initialize the system LAMEbus data */
	lamebus = lamebus_init();

	/* Probe CPUs (should these be done as device attachments instead?) */
	lamebus_find_cpus(lamebus);

	/*
	 * Print the device name for the main bus.
	 */
	kprintf("lamebus0 (system main bus)\n");

	/*
	 * Now we can take interrupts without croaking, so turn them on.
	 * Some device probes might require being able to get interrupts.
	 */

	spl0();

	/*
	 * Now probe all the devices attached to the bus.
	 * (This amounts to all devices.)
	 */
	autoconf_lamebus(lamebus, 0);

	/*
	 * Configure the MIPS on-chip timer to interrupt HZ times a second.
	 */
	mips_timer_set(CPU_FREQUENCY / HZ);
}

/*
 * Start all secondary CPUs.
 */
void
mainbus_start_cpus(void)
{
	lamebus_start_cpus(lamebus);
}

/*
 * Function to generate the memory address (in the uncached segment)
 * for the specified offset into the specified slot's region of the
 * LAMEbus.
 */
void *
lamebus_map_area(struct lamebus_softc *bus, int slot, uint32_t offset)
{
	uint32_t address;

	(void)bus;   // not needed

	KASSERT(slot >= 0 && slot < LB_NSLOTS);

	address = LB_BASEADDR + slot*LB_SLOT_SIZE + offset;
	return (void *)address;
}

/*
 * Read a 32-bit register from a LAMEbus device.
 */
uint32_t
lamebus_read_register(struct lamebus_softc *bus, int slot, uint32_t offset)
{
	uint32_t *ptr;

	ptr = lamebus_map_area(bus, slot, offset);

	/*
	 * Make sure the load happens after anything the device has
	 * been doing.
	 */
	membar_load_load();

	return *ptr;
}

/*
 * Write a 32-bit register of a LAMEbus device.
 */
void
lamebus_write_register(struct lamebus_softc *bus, int slot,
		       uint32_t offset, uint32_t val)
{
	uint32_t *ptr;

	ptr = lamebus_map_area(bus, slot, offset);
	*ptr = val;

	/*
	 * Make sure the store happens before we do anything else to
	 * the device.
	 */
	membar_store_store();
}


/*
 * Power off the system.
 */
void
mainbus_poweroff(void)
{
	/*
	 *
	 * Note that lamebus_write_register() doesn't actually access
	 * the bus argument, so this will still work if we get here
	 * before the bus is initialized.
	 */
	lamebus_poweroff(lamebus);
}

/*
 * Reboot the system.
 */
void
mainbus_reboot(void)
{
	/*
	 * The MIPS doesn't appear to have any on-chip reset.
	 * LAMEbus doesn't have a reset control, so we just
	 * power off instead of rebooting. This would not be
	 * so great in a real system, but it's fine for what
	 * we're doing.
	 */
	kprintf("Cannot reboot - powering off instead, sorry.\n");
	mainbus_poweroff();
}

/*
 * Halt the system.
 * On some systems, this would return to the boot monitor. But we don't
 * have one.
 */
void
mainbus_halt(void)
{
	cpu_halt();
}

/*
 * Called to reset the system from panic().
 *
 * By the time we get here, the system may well be sufficiently hosed
 * as to panic recursively if we do much of anything. So just power off.
 * (We'd reboot, but System/161 doesn't do that.)
 */
void
mainbus_panic(void)
{
	mainbus_poweroff();
}

/*
 * Function to get the size of installed physical RAM from the LAMEbus
 * controller.
 */
uint32_t
mainbus_ramsize(void)
{
	uint32_t ramsize;

	ramsize = lamebus_ramsize();

	/*
	 * This is the same as the last physical address, as long as
	 * we have less than 508 megabytes of memory. The LAMEbus I/O
	 * area occupies the space between 508 megabytes and 512
	 * megabytes, so if we had more RAM than this it would have to
	 * be discontiguous. This is not a case we are going to worry
	 * about.
	 */
	if (ramsize > 508*1024*1024) {
		ramsize = 508*1024*1024;
	}

	return ramsize;
}

/*
 * Send IPI.
 */
void
mainbus_send_ipi(struct cpu *target)
{
	lamebus_assert_ipi(lamebus, target);
}

/*
 * Trigger the debugger.
 */
void
mainbus_debugger(void)
{
	ltrace_stop(0);
}

/*
 * Interrupt dispatcher.
 */

/* Wiring of LAMEbus interrupts to bits in the cause register */
#define LAMEBUS_IRQ_BIT  0x00000400	/* all system bus slots */
#define LAMEBUS_IPI_BIT  0x00000800	/* inter-processor interrupt */
#define MIPS_TIMER_BIT   0x00008000	/* on-chip timer */

void
mainbus_interrupt(struct trapframe *tf)
{
	uint32_t cause;
	bool seen = false;

	/* interrupts should be off */
	KASSERT(curthread->t_curspl > 0);

	cause = tf->tf_cause;
	if (cause & LAMEBUS_IRQ_BIT) {
		lamebus_interrupt(lamebus);
		seen = true;
	}
	if (cause & LAMEBUS_IPI_BIT) {
		interprocessor_interrupt();
		lamebus_clear_ipi(lamebus, curcpu);
		seen = true;
	}
	if (cause & MIPS_TIMER_BIT) {
		/* Reset the timer (this clears the interrupt) */
		mips_timer_set(CPU_FREQUENCY / HZ);
		/* and call hardclock */
		hardclock();
		seen = true;
	}

	if (!seen) {
		if ((cause & CCA_IRQS) == 0) {
			/*
			 * Don't panic here; this can happen if an
			 * interrupt line asserts (very) briefly and
			 * turns off again before we get as far as
			 * reading the cause register.  This was
			 * actually seen... once.
			 */
		}
		else {
			/*
			 * But if we get an interrupt on an interrupt
			 * line that's not supposed to be wired up,
			 * complain.
			 */
			panic("Unknown interrupt; cause register is %08x\n",
			      cause);
		}
	}
}
