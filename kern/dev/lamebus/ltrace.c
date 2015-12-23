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
#include <platform/bus.h>
#include <lamebus/ltrace.h>
#include "autoconf.h"

/* Registers (offsets within slot) */
#define LTRACE_REG_TRON    0	/* trace on */
#define LTRACE_REG_TROFF   4	/* trace off */
#define LTRACE_REG_DEBUG   8	/* debug code */
#define LTRACE_REG_DUMP    12	/* dump the system */
#define LTRACE_REG_STOP    16	/* stop for the debugger */
#define LTRACE_REG_PROFEN  20	/* turn profiling on/off */
#define LTRACE_REG_PROFCL  24	/* clear the profile */

static struct ltrace_softc *the_trace;

void
ltrace_on(uint32_t code)
{
	if (the_trace != NULL) {
		bus_write_register(the_trace->lt_busdata, the_trace->lt_buspos,
				   LTRACE_REG_TRON, code);
	}
}

void
ltrace_off(uint32_t code)
{
	if (the_trace != NULL) {
		bus_write_register(the_trace->lt_busdata, the_trace->lt_buspos,
				   LTRACE_REG_TROFF, code);
	}
}

void
ltrace_debug(uint32_t code)
{
	if (the_trace != NULL) {
		bus_write_register(the_trace->lt_busdata, the_trace->lt_buspos,
				   LTRACE_REG_DEBUG, code);
	}
}

void
ltrace_dump(uint32_t code)
{
	if (the_trace != NULL) {
		bus_write_register(the_trace->lt_busdata, the_trace->lt_buspos,
				   LTRACE_REG_DUMP, code);
	}
}

void
ltrace_stop(uint32_t code)
{
	if (the_trace != NULL && the_trace->lt_canstop) {
		bus_write_register(the_trace->lt_busdata, the_trace->lt_buspos,
				   LTRACE_REG_STOP, code);
	}
}

void
ltrace_setprof(uint32_t onoff)
{
	if (the_trace != NULL && the_trace->lt_canprof) {
		bus_write_register(the_trace->lt_busdata, the_trace->lt_buspos,
				   LTRACE_REG_PROFEN, onoff);
	}
}

void
ltrace_eraseprof(void)
{
	if (the_trace != NULL && the_trace->lt_canprof) {
		bus_write_register(the_trace->lt_busdata, the_trace->lt_buspos,
				   LTRACE_REG_PROFCL, 1);
	}
}

int
config_ltrace(struct ltrace_softc *sc, int ltraceno)
{
	(void)ltraceno;
	the_trace = sc;
	return 0;
}
