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
#include <lib.h>
#include <spinlock.h>
#include <platform/bus.h>
#include <lamebus/lser.h>
#include "autoconf.h"

/* Registers (offsets within slot) */
#define LSER_REG_CHAR  0     /* Character in/out */
#define LSER_REG_WIRQ  4     /* Write interrupt status */
#define LSER_REG_RIRQ  8     /* Read interrupt status */

/* Bits in the IRQ registers */
#define LSER_IRQ_ENABLE  1
#define LSER_IRQ_ACTIVE  2
#define LSER_IRQ_FORCE   4

void
lser_irq(void *vsc)
{
	struct lser_softc *sc = vsc;
	uint32_t x;
	bool clear_to_write = false;
	bool got_a_read = false;
	uint32_t ch = 0;

	spinlock_acquire(&sc->ls_lock);

	x = bus_read_register(sc->ls_busdata, sc->ls_buspos, LSER_REG_WIRQ);
	if (x & LSER_IRQ_ACTIVE) {
		x = LSER_IRQ_ENABLE;
		sc->ls_wbusy = 0;
		clear_to_write = true;
		bus_write_register(sc->ls_busdata, sc->ls_buspos,
				   LSER_REG_WIRQ, x);
	}

	x = bus_read_register(sc->ls_busdata, sc->ls_buspos, LSER_REG_RIRQ);
	if (x & LSER_IRQ_ACTIVE) {
		x = LSER_IRQ_ENABLE;
		ch = bus_read_register(sc->ls_busdata, sc->ls_buspos,
				       LSER_REG_CHAR);
		got_a_read = true;
		bus_write_register(sc->ls_busdata, sc->ls_buspos,
				   LSER_REG_RIRQ, x);
	}

	spinlock_release(&sc->ls_lock);

	if (clear_to_write && sc->ls_start != NULL) {
		sc->ls_start(sc->ls_devdata);
	}
	if (got_a_read && sc->ls_input != NULL) {
		sc->ls_input(sc->ls_devdata, ch);
	}
}

void
lser_write(void *vls, int ch)
{
	struct lser_softc *ls = vls;

	spinlock_acquire(&ls->ls_lock);

	if (ls->ls_wbusy) {
		/*
		 * We're not clear to write.
		 *
		 * This should not happen. It's the job of the driver
		 * attached to us to not write until we call
		 * ls->ls_start.
		 *
		 * (Note: if we're the console, the panic will go to
		 * lser_writepolled for printing, because we hold a
		 * spinlock and interrupts are off; it won't recurse.)
		 */
		panic("lser: Not clear to write\n");
	}
	ls->ls_wbusy = true;

	bus_write_register(ls->ls_busdata, ls->ls_buspos, LSER_REG_CHAR, ch);

	spinlock_release(&ls->ls_lock);
}

static
void
lser_poll_until_write(struct lser_softc *sc)
{
	uint32_t val;

	KASSERT(spinlock_do_i_hold(&sc->ls_lock));

	do {
		val = bus_read_register(sc->ls_busdata, sc->ls_buspos,
					LSER_REG_WIRQ);
	}
	while ((val & LSER_IRQ_ACTIVE) == 0);
}

void
lser_writepolled(void *vsc, int ch)
{
	struct lser_softc *sc = vsc;
	bool irqpending;

	spinlock_acquire(&sc->ls_lock);

	if (sc->ls_wbusy) {
		irqpending = true;
		lser_poll_until_write(sc);
		/* Clear the ready condition, but leave the IRQ asserted */
		bus_write_register(sc->ls_busdata, sc->ls_buspos,
				   LSER_REG_WIRQ,
				   LSER_IRQ_FORCE|LSER_IRQ_ENABLE);
	}
	else {
		irqpending = false;
		/* Clear the interrupt enable bit */
		bus_write_register(sc->ls_busdata, sc->ls_buspos,
				   LSER_REG_WIRQ, 0);
	}

	/* Send the character. */
	bus_write_register(sc->ls_busdata, sc->ls_buspos, LSER_REG_CHAR, ch);

	/* Wait until it's done. */
	lser_poll_until_write(sc);

	/*
	 * If there wasn't already an IRQ pending, clear the ready
	 * condition and turn interruption back on. But if there was,
	 * leave the register alone, with the ready condition set (and
	 * the force bit still on); in due course we'll get to the
	 * interrupt handler and they'll be cleared.
	 */
	if (!irqpending) {
		bus_write_register(sc->ls_busdata, sc->ls_buspos,
				   LSER_REG_WIRQ, LSER_IRQ_ENABLE);
	}

	spinlock_release(&sc->ls_lock);
}

int
config_lser(struct lser_softc *sc, int lserno)
{
	(void)lserno;

	/*
	 * Enable interrupting.
	 */

	spinlock_init(&sc->ls_lock);
	sc->ls_wbusy = false;

	bus_write_register(sc->ls_busdata, sc->ls_buspos,
			   LSER_REG_RIRQ, LSER_IRQ_ENABLE);
	bus_write_register(sc->ls_busdata, sc->ls_buspos,
			   LSER_REG_WIRQ, LSER_IRQ_ENABLE);

	return 0;
}
