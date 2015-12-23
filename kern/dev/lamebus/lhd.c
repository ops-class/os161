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
 * LAMEbus hard disk (lhd) driver.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <uio.h>
#include <membar.h>
#include <synch.h>
#include <platform/bus.h>
#include <vfs.h>
#include <lamebus/lhd.h>
#include "autoconf.h"

/* Registers (offsets within slot) */
#define LHD_REG_NSECT   0   /* Number of sectors */
#define LHD_REG_STAT    4   /* Status */
#define LHD_REG_SECT    8   /* Sector for I/O */
#define LHD_REG_RPM     12  /* Disk rotation speed (revs per minute) */

/* Status codes */
#define LHD_IDLE        0   /* Device idle */
#define LHD_WORKING     1   /* Operation in progress */
#define LHD_OK          4   /* Operation succeeded */
#define LHD_INVSECT     12  /* Invalid sector requested */
#define LHD_MEDIA       20  /* Media error */
#define LHD_ISWRITE     2   /* OR with above: I/O is a write */
#define LHD_STATEMASK   0x1d  /* mask for masking out LHD_ISWRITE */

/* Buffer (offset within slot)  */
#define LHD_BUFFER      32768

/*
 * Shortcut for reading a register.
 */
static
inline
uint32_t lhd_rdreg(struct lhd_softc *lh, uint32_t reg)
{
	return bus_read_register(lh->lh_busdata, lh->lh_buspos, reg);
}

/*
 * Shortcut for writing a register.
 */
static
inline
void lhd_wreg(struct lhd_softc *lh, uint32_t reg, uint32_t val)
{
	bus_write_register(lh->lh_busdata, lh->lh_buspos, reg, val);
}

/*
 * Convert a result code from the hardware to an errno value.
 */
static
int lhd_code_to_errno(struct lhd_softc *lh, int code)
{
	switch (code & LHD_STATEMASK) {
	    case LHD_OK: return 0;
	    case LHD_INVSECT: return EINVAL;
	    case LHD_MEDIA: return EIO;
	}
	kprintf("lhd%d: Unknown result code %d\n", lh->lh_unit, code);
	return EAGAIN;
}

/*
 * Record that an I/O has completed: save the result and poke the
 * completion semaphore.
 */
static
void
lhd_iodone(struct lhd_softc *lh, int err)
{
	lh->lh_result = err;
	V(lh->lh_done);
}

/*
 * Interrupt handler for lhd.
 * Read the status register; if an operation finished, clear the status
 * register and report completion.
 */
void
lhd_irq(void *vlh)
{
	struct lhd_softc *lh = vlh;
	uint32_t val;

	val = lhd_rdreg(lh, LHD_REG_STAT);

	switch (val & LHD_STATEMASK) {
	    case LHD_IDLE:
	    case LHD_WORKING:
		break;
	    case LHD_OK:
	    case LHD_INVSECT:
	    case LHD_MEDIA:
		lhd_wreg(lh, LHD_REG_STAT, 0);
		lhd_iodone(lh, lhd_code_to_errno(lh, val));
		break;
	}
}

/*
 * Function called when we are open()'d.
 */
static
int
lhd_eachopen(struct device *d, int openflags)
{
	/*
	 * Don't need to do anything.
	 */
	(void)d;
	(void)openflags;

	return 0;
}

/*
 * Function for handling ioctls.
 */
static
int
lhd_ioctl(struct device *d, int op, userptr_t data)
{
	/*
	 * We don't support any ioctls.
	 */
	(void)d;
	(void)op;
	(void)data;
	return EIOCTL;
}

#if 0
/*
 * Reset the device.
 * This could be used, for instance, on timeout, if you implement suitable
 * facilities.
 */
static
void
lhd_reset(struct lhd_softc *lh)
{
	lhd_wreg(lh, LHD_REG_STAT, 0);
}
#endif

/*
 * I/O function (for both reads and writes)
 */
static
int
lhd_io(struct device *d, struct uio *uio)
{
	struct lhd_softc *lh = d->d_data;

	uint32_t sector = uio->uio_offset / LHD_SECTSIZE;
	uint32_t sectoff = uio->uio_offset % LHD_SECTSIZE;
	uint32_t len = uio->uio_resid / LHD_SECTSIZE;
	uint32_t lenoff = uio->uio_resid % LHD_SECTSIZE;
	uint32_t i;
	uint32_t statval = LHD_WORKING;
	int result;

	/* Don't allow I/O that isn't sector-aligned. */
	if (sectoff != 0 || lenoff != 0) {
		return EINVAL;
	}

	/* Don't allow I/O past the end of the disk. */
	/* XXX this check can overflow */
	if (sector+len > lh->lh_dev.d_blocks) {
		return EINVAL;
	}

	/* Set up the value to write into the status register. */
	if (uio->uio_rw==UIO_WRITE) {
		statval |= LHD_ISWRITE;
	}

	/* Loop over all the sectors we were asked to do. */
	for (i=0; i<len; i++) {

		/* Wait until nobody else is using the device. */
		P(lh->lh_clear);

		/*
		 * Are we writing? If so, transfer the data to the
		 * on-card buffer.
		 */
		if (uio->uio_rw == UIO_WRITE) {
			result = uiomove(lh->lh_buf, LHD_SECTSIZE, uio);
			membar_store_store();
			if (result) {
				V(lh->lh_clear);
				return result;
			}
		}

		/* Tell it what sector we want... */
		lhd_wreg(lh, LHD_REG_SECT, sector+i);

		/* and start the operation. */
		lhd_wreg(lh, LHD_REG_STAT, statval);

		/* Now wait until the interrupt handler tells us we're done. */
		P(lh->lh_done);

		/* Get the result value saved by the interrupt handler. */
		result = lh->lh_result;

		/*
		 * Are we reading? If so, and if we succeeded,
		 * transfer the data out of the on-card buffer.
		 */
		if (result==0 && uio->uio_rw==UIO_READ) {
			membar_load_load();
			result = uiomove(lh->lh_buf, LHD_SECTSIZE, uio);
		}

		/* Tell another thread it's cleared to go ahead. */
		V(lh->lh_clear);

		/* If we failed, return the error. */
		if (result) {
			return result;
		}
	}

	return 0;
}

static const struct device_ops lhd_devops = {
	.devop_eachopen = lhd_eachopen,
	.devop_io = lhd_io,
	.devop_ioctl = lhd_ioctl,
};

/*
 * Setup routine called by autoconf.c when an lhd is found.
 */
int
config_lhd(struct lhd_softc *lh, int lhdno)
{
	char name[32];

	/* Figure out what our name is. */
	snprintf(name, sizeof(name), "lhd%d", lhdno);

	/* Get a pointer to the on-chip buffer. */
	lh->lh_buf = bus_map_area(lh->lh_busdata, lh->lh_buspos, LHD_BUFFER);

	/* Create the semaphores. */
	lh->lh_clear = sem_create("lhd-clear", 1);
	if (lh->lh_clear == NULL) {
		return ENOMEM;
	}
	lh->lh_done = sem_create("lhd-done", 0);
	if (lh->lh_done == NULL) {
		sem_destroy(lh->lh_clear);
		lh->lh_clear = NULL;
		return ENOMEM;
	}

	/* Set up the VFS device structure. */
	lh->lh_dev.d_ops = &lhd_devops;
	lh->lh_dev.d_blocks = bus_read_register(lh->lh_busdata, lh->lh_buspos,
						LHD_REG_NSECT);
	lh->lh_dev.d_blocksize = LHD_SECTSIZE;
	lh->lh_dev.d_data = lh;

	/* Add the VFS device structure to the VFS device list. */
	return vfs_adddev(name, &lh->lh_dev, 1);
}
