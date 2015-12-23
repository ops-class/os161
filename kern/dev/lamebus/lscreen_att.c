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
 * Code for probe/attach of lscreen to LAMEbus.
 */
#include <types.h>
#include <lib.h>
#include <lamebus/lamebus.h>
#include <lamebus/lscreen.h>
#include "autoconf.h"

/* Lowest revision we support */
#define LOW_VERSION   1
/* Highest revision we support */
#define HIGH_VERSION  1

struct lscreen_softc *
attach_lscreen_to_lamebus(int lscreenno, struct lamebus_softc *sc)
{
	struct lscreen_softc *ls;
	int slot = lamebus_probe(sc, LB_VENDOR_CS161, LBCS161_SCREEN,
				 LOW_VERSION, HIGH_VERSION);
	if (slot < 0) {
		/* Not found */
		return NULL;
	}

	ls = kmalloc(sizeof(struct lscreen_softc));
	if (ls==NULL) {
		/* Out of memory */
		return NULL;
	}

	/* Record what it's attached to */
	ls->ls_busdata = sc;
	ls->ls_buspos = slot;

	/* Mark the slot in use and hook the interrupt */
	lamebus_mark(sc, slot);
	lamebus_attach_interrupt(sc, slot, ls, lscreen_irq);

	return ls;
}
