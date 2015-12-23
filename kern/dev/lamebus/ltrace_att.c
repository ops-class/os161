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
#include <lamebus/lamebus.h>
#include <lamebus/ltrace.h>
#include "autoconf.h"

/* Lowest revision we support */
#define LOW_VERSION   1
/* Revision that supports ltrace_stop() */
#define STOP_VERSION  2
/* Revision that supports ltrace_prof() */
#define PROF_VERSION  3

struct ltrace_softc *
attach_ltrace_to_lamebus(int ltraceno, struct lamebus_softc *sc)
{
	struct ltrace_softc *lt;
	uint32_t drl;
	int slot = lamebus_probe(sc, LB_VENDOR_CS161, LBCS161_TRACE,
				 LOW_VERSION, &drl);
	if (slot < 0) {
		return NULL;
	}

	lt = kmalloc(sizeof(struct ltrace_softc));
	if (lt==NULL) {
		return NULL;
	}

	(void)ltraceno;  // unused

	lt->lt_busdata = sc;
	lt->lt_buspos = slot;
	lt->lt_canstop = drl >= STOP_VERSION;
	lt->lt_canprof = drl >= PROF_VERSION;

	lamebus_mark(sc, slot);

	return lt;
}
