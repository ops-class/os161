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
 * Driver for LAMEbus random generator card
 */
#include <types.h>
#include <lib.h>
#include <uio.h>
#include <platform/bus.h>
#include <lamebus/lrandom.h>
#include "autoconf.h"

/* Registers (offsets within slot) */
#define LR_REG_RAND   0     /* random register */

/* Constants */
#define LR_RANDMAX  0xffffffff

int
config_lrandom(struct lrandom_softc *lr, int lrandomno)
{
	(void)lrandomno;
	(void)lr;
	return 0;
}

uint32_t
lrandom_random(void *devdata)
{
	struct lrandom_softc *lr = devdata;
	return bus_read_register(lr->lr_bus, lr->lr_buspos, LR_REG_RAND);
}

uint32_t
lrandom_randmax(void *devdata)
{
	(void)devdata;
	return LR_RANDMAX;
}

int
lrandom_read(void *devdata, struct uio *uio)
{
	struct lrandom_softc *lr = devdata;
	uint32_t val;
	int result;

	while (uio->uio_resid > 0) {
		val = bus_read_register(lr->lr_bus, lr->lr_buspos,
					  LR_REG_RAND);
		result = uiomove(&val, sizeof(val), uio);
		if (result) {
			return result;
		}
	}

	return 0;
}
