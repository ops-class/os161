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

#ifndef _LAMEBUS_H_
#define _LAMEBUS_H_

#include <cpu.h>
#include <spinlock.h>

/*
 * Linear Always Mapped Extents
 *
 * Machine-independent definitions.
 */


/* Vendors */
#define LB_VENDOR_CS161      1

/* CS161 devices */
#define LBCS161_UPBUSCTL     1
#define LBCS161_TIMER        2
#define LBCS161_DISK         3
#define LBCS161_SERIAL       4
#define LBCS161_SCREEN       5
#define LBCS161_NET          6
#define LBCS161_EMUFS        7
#define LBCS161_TRACE        8
#define LBCS161_RANDOM       9
#define LBCS161_MPBUSCTL     10

/* LAMEbus controller always goes in slot 31 */
#define LB_CONTROLLER_SLOT   31

/* Number of slots */
#define LB_NSLOTS            32

/* LAMEbus controller per-slot config space */
#define LB_CONFIG_SIZE       1024

/* LAMEbus controller per-cpu control space */
#define LB_CTLCPU_SIZE       1024

/* LAMEbus controller slot offset to per-cpu control space */
#define LB_CTLCPU_OFFSET     32768

/* LAMEbus mapping size per slot */
#define LB_SLOT_SIZE         65536

/* Pointer to kind of function called on interrupt */
typedef void (*lb_irqfunc)(void *devdata);

/*
 * Driver data
 */
struct lamebus_softc {
	struct spinlock ls_lock;

	/* Accessed from interrupts; synchronized with ls_lock */
	uint32_t     ls_slotsinuse;
	void        *ls_devdata[LB_NSLOTS];
	lb_irqfunc   ls_irqfuncs[LB_NSLOTS];

	/* Read-only once set early in boot */
	unsigned     ls_uniprocessor;
};

/*
 * Allocate and set up a lamebus_softc for the system.
 */
struct lamebus_softc *lamebus_init(void);

/*
 * Search for and create cpu structures for the CPUs on the mainboard.
 */
void lamebus_find_cpus(struct lamebus_softc *lamebus);

/*
 * Start up secondary CPUs.
 */
void lamebus_start_cpus(struct lamebus_softc *lamebus);

/*
 * Look for a not-in-use slot containing a device whose vendor and device
 * ids match those provided, and whose version is in the range between
 * lowver and highver, inclusive.
 *
 * Returns a slot number (0-31) or -1 if no such device is found.
 */
int lamebus_probe(struct lamebus_softc *,
		  uint32_t vendorid, uint32_t deviceid,
		  uint32_t lowver, uint32_t *version_ret);

/*
 * Mark a slot in-use (that is, has a device driver attached to it),
 * or unmark it. It is a fatal error to mark a slot that is already
 * in use, or unmark a slot that is not in use.
 */
void lamebus_mark(struct lamebus_softc *, int slot);
void lamebus_unmark(struct lamebus_softc *, int slot);

/*
 * Attach to an interrupt.
 */
void lamebus_attach_interrupt(struct lamebus_softc *, int slot,
			      void *devdata,
			      void (*irqfunc)(void *devdata));
/*
 * Detach from interrupt.
 */
void lamebus_detach_interrupt(struct lamebus_softc *, int slot);

/*
 * Mask/unmask an interrupt.
 */
void lamebus_mask_interrupt(struct lamebus_softc *, int slot);
void lamebus_unmask_interrupt(struct lamebus_softc *, int slot);

/*
 * Function to call to handle a LAMEbus interrupt.
 */
void lamebus_interrupt(struct lamebus_softc *);

/*
 * Have the LAMEbus controller power the system off.
 */
void lamebus_poweroff(struct lamebus_softc *);

/*
 * Ask the bus controller how much memory we have.
 */
size_t lamebus_ramsize(void);

/*
 * Turn on or off the inter-processor interrupt line to a CPU.
 */
void lamebus_assert_ipi(struct lamebus_softc *, struct cpu *targetcpu);
void lamebus_clear_ipi(struct lamebus_softc *, struct cpu *targetcpu);

/*
 * Read/write 32-bit register at offset OFFSET within slot SLOT.
 * (Machine dependent.)
 */
uint32_t lamebus_read_register(struct lamebus_softc *, int slot,
			       uint32_t offset);
void lamebus_write_register(struct lamebus_softc *, int slot,
			    uint32_t offset, uint32_t val);

/*
 * Map a buffer that starts at offset OFFSET within slot SLOT.
 */
void *lamebus_map_area(struct lamebus_softc *, int slot,
		       uint32_t offset);


#endif /* _LAMEBUS_H_ */
