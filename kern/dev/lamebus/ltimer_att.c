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
 * Routine for probing/attaching ltimer to LAMEbus.
 */
#include <types.h>
#include <lib.h>
#include <lamebus/lamebus.h>
#include <lamebus/ltimer.h>
#include "autoconf.h"

/* Lowest revision we support */
#define LOW_VERSION   1

struct ltimer_softc *
attach_ltimer_to_lamebus(int ltimerno, struct lamebus_softc *sc)
{
	struct ltimer_softc *lt;
	int slot = lamebus_probe(sc, LB_VENDOR_CS161, LBCS161_TIMER,
				 LOW_VERSION, NULL);
	if (slot < 0) {
		/* No ltimer (or no additional ltimer) found */
		return NULL;
	}

	lt = kmalloc(sizeof(struct ltimer_softc));
	if (lt==NULL) {
		/* out of memory */
		return NULL;
	}

	(void)ltimerno;  // unused

	/* Record what bus it's on */
	lt->lt_bus = sc;
	lt->lt_buspos = slot;

	/* Mark the slot in use and hook that slot's interrupt */
	lamebus_mark(sc, slot);
	lamebus_attach_interrupt(sc, slot, lt, ltimer_irq);

	return lt;
}
