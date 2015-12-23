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
 * Machine-independent LAMEbus code.
 */

#include <types.h>
#include <lib.h>
#include <cpu.h>
#include <membar.h>
#include <spinlock.h>
#include <current.h>
#include <lamebus/lamebus.h>

/* Register offsets within each config region */
#define CFGREG_VID   0    /* Vendor ID */
#define CFGREG_DID   4    /* Device ID */
#define CFGREG_DRL   8    /* Device Revision Level */

/* LAMEbus controller private registers (offsets within its config region) */
#define CTLREG_RAMSZ    0x200
#define CTLREG_IRQS     0x204
#define CTLREG_PWR      0x208
#define CTLREG_IRQE     0x20c
#define CTLREG_CPUS     0x210
#define CTLREG_CPUE     0x214
#define CTLREG_SELF     0x218

/* LAMEbus CPU control registers (offsets within each per-cpu region) */
#define CTLCPU_CIRQE	0x000
#define CTLCPU_CIPI	0x004
#define CTLCPU_CRAM	0x300


/*
 * Read a config register for the given slot.
 */
static
inline
uint32_t
read_cfg_register(struct lamebus_softc *lb, int slot, uint32_t offset)
{
	/* Note that lb might be NULL on some platforms in some contexts. */
	offset += LB_CONFIG_SIZE*slot;
	return lamebus_read_register(lb, LB_CONTROLLER_SLOT, offset);
}

/*
 * Write a config register for a given slot.
 */
static
inline
void
write_cfg_register(struct lamebus_softc *lb, int slot, uint32_t offset,
		   uint32_t val)
{
	offset += LB_CONFIG_SIZE*slot;
	lamebus_write_register(lb, LB_CONTROLLER_SLOT, offset, val);
}

/*
 * Read one of the bus controller's registers.
 */
static
inline
uint32_t
read_ctl_register(struct lamebus_softc *lb, uint32_t offset)
{
	/* Note that lb might be NULL on some platforms in some contexts. */
	return read_cfg_register(lb, LB_CONTROLLER_SLOT, offset);
}

/*
 * Write one of the bus controller's registers.
 */
static
inline
void
write_ctl_register(struct lamebus_softc *lb, uint32_t offset, uint32_t val)
{
	write_cfg_register(lb, LB_CONTROLLER_SLOT, offset, val);
}

/*
 * Write one of the bus controller's CPU control registers.
 */
static
inline
void
write_ctlcpu_register(struct lamebus_softc *lb, unsigned hw_cpunum,
		      uint32_t offset, uint32_t val)
{
	offset += LB_CTLCPU_OFFSET + hw_cpunum * LB_CTLCPU_SIZE;
	lamebus_write_register(lb, LB_CONTROLLER_SLOT, offset, val);
}

/*
 * Find and create secondary CPUs.
 */
void
lamebus_find_cpus(struct lamebus_softc *lamebus)
{
	uint32_t mainboard_vid, mainboard_did;
	uint32_t cpumask, self, bit, val;
	unsigned i, numcpus, bootcpu;
	unsigned hwnum[32];

	mainboard_vid = read_cfg_register(lamebus, LB_CONTROLLER_SLOT,
					  CFGREG_VID);
	mainboard_did = read_cfg_register(lamebus, LB_CONTROLLER_SLOT,
					  CFGREG_DID);
	if (mainboard_vid == LB_VENDOR_CS161 &&
	    mainboard_did == LBCS161_UPBUSCTL) {
		/* Old uniprocessor mainboard; no cpu registers. */
		lamebus->ls_uniprocessor = 1;
		return;
	}

	cpumask = read_ctl_register(lamebus, CTLREG_CPUS);
	self = read_ctl_register(lamebus, CTLREG_SELF);

	numcpus = 0;
	bootcpu = 0;
	for (i=0; i<32; i++) {
		bit = (uint32_t)1 << i;
		if ((cpumask & bit) != 0) {
			if (self & bit) {
				bootcpu = numcpus;
				curcpu->c_hardware_number = i;
			}
			hwnum[numcpus] = i;
			numcpus++;
		}
	}

	for (i=0; i<numcpus; i++) {
		if (i != bootcpu) {
			cpu_create(hwnum[i]);
		}
	}

	/*
	 * By default, route all interrupts only to the boot cpu. We
	 * could be arbitrarily more elaborate, up to things like
	 * dynamic load balancing.
	 */

	for (i=0; i<numcpus; i++) {
		if (i != bootcpu) {
			val = 0;
		}
		else {
			val = 0xffffffff;
		}
		write_ctlcpu_register(lamebus, hwnum[i], CTLCPU_CIRQE, val);
	}
}

/*
 * Start up secondary CPUs.
 *
 * The first word of the CRAM area is set to the entry point for new
 * CPUs; the second to the (software) CPU number. Note that the logic
 * here assumes the boot CPU is CPU 0 and the others are 1-N as
 * created in the function above. This is fine if all CPUs are on
 * LAMEbus; if in some environment there are other CPUs about as well
 * this logic will have to be made more complex.
 */
void
lamebus_start_cpus(struct lamebus_softc *lamebus)
{
	uint32_t cpumask, self, bit;
	uint32_t ctlcpuoffset;
	uint32_t *cram;
	unsigned i;
	unsigned cpunum;

	if (lamebus->ls_uniprocessor) {
		return;
	}

	cpumask = read_ctl_register(lamebus, CTLREG_CPUS);
	self = read_ctl_register(lamebus, CTLREG_SELF);

	/* Poke in the startup address. */
	cpunum = 1;
	for (i=0; i<32; i++) {
		bit = (uint32_t)1 << i;
		if ((cpumask & bit) != 0) {
			if (self & bit) {
				continue;
			}
			ctlcpuoffset = LB_CTLCPU_OFFSET + i * LB_CTLCPU_SIZE;
			cram = lamebus_map_area(lamebus,
						LB_CONTROLLER_SLOT,
						ctlcpuoffset + CTLCPU_CRAM);
			cram[0] = (uint32_t)cpu_start_secondary;
			cram[1] = cpunum++;
		}
	}
	/* Ensure all the above writes get flushed. */
	membar_store_store();

	/* Now, enable them all. */
	write_ctl_register(lamebus, CTLREG_CPUE, cpumask);
}

/*
 * Probe function.
 *
 * Given a LAMEbus, look for a device that's not already been marked
 * in use, has the specified IDs, and has a device revision level at
 * least as high as the minimum specified.
 *
 * Returns the slot number found (0-31) or -1 if nothing suitable was
 * found.
 *
 * If VERSION_RET is not null, return the device version found. This
 * allows drivers to blacklist specific versions or otherwise conduct
 * more specific checks.
 */

int
lamebus_probe(struct lamebus_softc *sc,
	      uint32_t vendorid, uint32_t deviceid,
	      uint32_t lowver, uint32_t *version_ret)
{
	int slot;
	uint32_t val;

	/*
	 * Because the slot information in sc is used when dispatching
	 * interrupts, disable interrupts while working with it.
	 */

	spinlock_acquire(&sc->ls_lock);

	for (slot=0; slot<LB_NSLOTS; slot++) {
		if (sc->ls_slotsinuse & (1<<slot)) {
			/* Slot already in use; skip */
			continue;
		}

		val = read_cfg_register(sc, slot, CFGREG_VID);
		if (val!=vendorid) {
			/* Wrong vendor id */
			continue;
		}

		val = read_cfg_register(sc, slot, CFGREG_DID);
		if (val != deviceid) {
			/* Wrong device id */
			continue;
		}

		val = read_cfg_register(sc, slot, CFGREG_DRL);
		if (val < lowver) {
			/* Unsupported device revision */
			continue;
		}
		if (version_ret != NULL) {
			*version_ret = val;
		}

		/* Found something */

		spinlock_release(&sc->ls_lock);
		return slot;
	}

	/* Found nothing */

	spinlock_release(&sc->ls_lock);
	return -1;
}

/*
 * Mark that a slot is in use.
 * This prevents the probe routine from returning the same device over
 * and over again.
 */
void
lamebus_mark(struct lamebus_softc *sc, int slot)
{
	uint32_t mask = ((uint32_t)1) << slot;
	KASSERT(slot>=0 && slot < LB_NSLOTS);

	spinlock_acquire(&sc->ls_lock);

	if ((sc->ls_slotsinuse & mask)!=0) {
		panic("lamebus_mark: slot %d already in use\n", slot);
	}

	sc->ls_slotsinuse |= mask;

	spinlock_release(&sc->ls_lock);
}

/*
 * Mark that a slot is no longer in use.
 */
void
lamebus_unmark(struct lamebus_softc *sc, int slot)
{
	uint32_t mask = ((uint32_t)1) << slot;
	KASSERT(slot>=0 && slot < LB_NSLOTS);

	spinlock_acquire(&sc->ls_lock);

	if ((sc->ls_slotsinuse & mask)==0) {
		panic("lamebus_mark: slot %d not marked in use\n", slot);
	}

	sc->ls_slotsinuse &= ~mask;

	spinlock_release(&sc->ls_lock);
}

/*
 * Register a function (and a device context pointer) to be called
 * when a particular slot signals an interrupt.
 */
void
lamebus_attach_interrupt(struct lamebus_softc *sc, int slot,
			 void *devdata,
			 void (*irqfunc)(void *devdata))
{
	uint32_t mask = ((uint32_t)1) << slot;
	KASSERT(slot>=0 && slot < LB_NSLOTS);

	spinlock_acquire(&sc->ls_lock);

	if ((sc->ls_slotsinuse & mask)==0) {
		panic("lamebus_attach_interrupt: slot %d not marked in use\n",
		      slot);
	}

	KASSERT(sc->ls_devdata[slot]==NULL);
	KASSERT(sc->ls_irqfuncs[slot]==NULL);

	sc->ls_devdata[slot] = devdata;
	sc->ls_irqfuncs[slot] = irqfunc;

	spinlock_release(&sc->ls_lock);
}

/*
 * Unregister a function that was being called when a particular slot
 * signaled an interrupt.
 */
void
lamebus_detach_interrupt(struct lamebus_softc *sc, int slot)
{
	uint32_t mask = ((uint32_t)1) << slot;
	KASSERT(slot>=0 && slot < LB_NSLOTS);

	spinlock_acquire(&sc->ls_lock);

	if ((sc->ls_slotsinuse & mask)==0) {
		panic("lamebus_detach_interrupt: slot %d not marked in use\n",
		      slot);
	}

	KASSERT(sc->ls_irqfuncs[slot]!=NULL);

	sc->ls_devdata[slot] = NULL;
	sc->ls_irqfuncs[slot] = NULL;

	spinlock_release(&sc->ls_lock);
}

/*
 * Mask/unmask an interrupt using the global IRQE register.
 */
void
lamebus_mask_interrupt(struct lamebus_softc *lamebus, int slot)
{
	uint32_t bits, mask = ((uint32_t)1) << slot;
	KASSERT(slot >= 0 && slot < LB_NSLOTS);

	spinlock_acquire(&lamebus->ls_lock);
	bits = read_ctl_register(lamebus, CTLREG_IRQE);
	bits &= ~mask;
	write_ctl_register(lamebus, CTLREG_IRQE, bits);
	spinlock_release(&lamebus->ls_lock);
}

void
lamebus_unmask_interrupt(struct lamebus_softc *lamebus, int slot)
{
	uint32_t bits, mask = ((uint32_t)1) << slot;
	KASSERT(slot >= 0 && slot < LB_NSLOTS);

	spinlock_acquire(&lamebus->ls_lock);
	bits = read_ctl_register(lamebus, CTLREG_IRQE);
	bits |= mask;
	write_ctl_register(lamebus, CTLREG_IRQE, bits);
	spinlock_release(&lamebus->ls_lock);
}


/*
 * LAMEbus interrupt handling function. (Machine-independent!)
 */
void
lamebus_interrupt(struct lamebus_softc *lamebus)
{
	/*
	 * Note that despite the fact that "spl" stands for "set
	 * priority level", we don't actually support interrupt
	 * priorities. When an interrupt happens, we look through the
	 * slots to find the first interrupting device and call its
	 * interrupt routine, no matter what that device is.
	 *
	 * Note that the entire LAMEbus uses only one on-cpu interrupt line.
	 * Thus, we do not use any on-cpu interrupt priority system either.
	 */

	int slot;
	uint32_t mask;
	uint32_t irqs;
	void (*handler)(void *);
	void *data;

	/* For keeping track of how many bogus things happen in a row. */
	static int duds = 0;
	int duds_this_time = 0;

	/* and we better have a valid bus instance. */
	KASSERT(lamebus != NULL);

	/* Lock the softc */
	spinlock_acquire(&lamebus->ls_lock);

	/*
	 * Read the LAMEbus controller register that tells us which
	 * slots are asserting an interrupt condition.
	 */
	irqs = read_ctl_register(lamebus, CTLREG_IRQS);

	if (irqs == 0) {
		/*
		 * Huh? None of them? Must be a glitch.
		 */
		kprintf("lamebus: stray interrupt on cpu %u\n",
			curcpu->c_number);
		duds++;
		duds_this_time++;

		/*
		 * We could just return now, but instead we'll
		 * continue ahead. Because irqs == 0, nothing in the
		 * loop will execute, and passing through it gets us
		 * to the code that checks how many duds we've
		 * seen. This is important, because we just might get
		 * a stray interrupt that latches itself on. If that
		 * happens, we're pretty much toast, but it's better
		 * to panic and hopefully reset the system than to
		 * loop forever printing "stray interrupt".
		 */
	}

	/*
	 * Go through the bits in the value we got back to see which
	 * ones are set.
	 */

	for (mask=1, slot=0; slot<LB_NSLOTS; mask<<=1, slot++) {
		if ((irqs & mask) == 0) {
			/* Nope. */
			continue;
		}

		/*
		 * This slot is signalling an interrupt.
		 */

		if ((lamebus->ls_slotsinuse & mask)==0) {
			/*
			 * No device driver is using this slot.
			 */
			duds++;
			duds_this_time++;
			continue;
		}

		if (lamebus->ls_irqfuncs[slot]==NULL) {
			/*
			 * The device driver hasn't installed an interrupt
			 * handler.
			 */
			duds++;
			duds_this_time++;
			continue;
		}

		/*
		 * Call the interrupt handler. Release the spinlock
		 * while we do so, in case other CPUs are handling
		 * interrupts on other devices.
		 */
		handler = lamebus->ls_irqfuncs[slot];
		data = lamebus->ls_devdata[slot];
		spinlock_release(&lamebus->ls_lock);

		handler(data);

		spinlock_acquire(&lamebus->ls_lock);

		/*
		 * Reload the mask of pending IRQs - if we just called
		 * hardclock, we might not have come back to this
		 * context for some time, and it might have changed.
		 */

		irqs = read_ctl_register(lamebus, CTLREG_IRQS);
	}


	/*
	 * If we get interrupts for a slot with no driver or no
	 * interrupt handler, it's fairly serious. Because LAMEbus
	 * uses level-triggered interrupts, if we don't shut off the
	 * condition, we'll keep getting interrupted continuously and
	 * the system will make no progress. But we don't know how to
	 * do that if there's no driver or no interrupt handler.
	 *
	 * So, if we get too many dud interrupts, panic, since it's
	 * better to panic and reset than to hang.
	 *
	 * If we get through here without seeing any duds this time,
	 * the condition, whatever it was, has gone away. It might be
	 * some stupid device we don't have a driver for, or it might
	 * have been an electrical transient. In any case, warn and
	 * clear the dud count.
	 */

	if (duds_this_time == 0 && duds > 0) {
		kprintf("lamebus: %d dud interrupts\n", duds);
		duds = 0;
	}

	if (duds > 10000) {
		panic("lamebus: too many (%d) dud interrupts\n", duds);
	}

	/* Unlock the softc */
	spinlock_release(&lamebus->ls_lock);
}

/*
 * Have the bus controller power the system off.
 */
void
lamebus_poweroff(struct lamebus_softc *lamebus)
{
	/*
	 * Write 0 to the power register to shut the system off.
	 */

	cpu_irqoff();
	write_ctl_register(lamebus, CTLREG_PWR, 0);

	/* The power doesn't go off instantly... so halt the cpu. */
	cpu_halt();
}

/*
 * Ask the bus controller how much memory we have.
 */
uint32_t
lamebus_ramsize(void)
{
	/*
	 * Note that this has to work before bus initialization.
	 * On machines where lamebus_read_register doesn't work
	 * before bus initialization, this function can't be used
	 * for initial RAM size lookup.
	 */

	return read_ctl_register(NULL, CTLREG_RAMSZ);
}

/*
 * Turn on or off the interprocessor interrupt line for a given CPU.
 */
void
lamebus_assert_ipi(struct lamebus_softc *lamebus, struct cpu *target)
{
	if (lamebus->ls_uniprocessor) {
		return;
	}
	write_ctlcpu_register(lamebus, target->c_hardware_number,
			      CTLCPU_CIPI, 1);
}

void
lamebus_clear_ipi(struct lamebus_softc *lamebus, struct cpu *target)
{
	if (lamebus->ls_uniprocessor) {
		return;
	}
	write_ctlcpu_register(lamebus, target->c_hardware_number,
			      CTLCPU_CIPI, 0);
}

/*
 * Initial setup.
 * Should be called from mainbus_bootstrap().
 */
struct lamebus_softc *
lamebus_init(void)
{
	struct lamebus_softc *lamebus;
	int i;

	/* Allocate space for lamebus data */
	lamebus = kmalloc(sizeof(struct lamebus_softc));
	if (lamebus==NULL) {
		panic("lamebus_init: Out of memory\n");
	}

	spinlock_init(&lamebus->ls_lock);

	/*
	 * Initialize the LAMEbus data structure.
	 */
	lamebus->ls_slotsinuse = 1 << LB_CONTROLLER_SLOT;

	for (i=0; i<LB_NSLOTS; i++) {
		lamebus->ls_devdata[i] = NULL;
		lamebus->ls_irqfuncs[i] = NULL;
	}

	lamebus->ls_uniprocessor = 0;

	return lamebus;
}
