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

#ifndef _LAMEBUS_LSER_H_
#define _LAMEBUS_LSER_H_

#include <spinlock.h>

struct lser_softc {
	/* Initialized by config function */
	struct spinlock ls_lock;    /* protects ls_wbusy and device regs */
	volatile bool ls_wbusy;     /* true if write in progress */

	/* Initialized by lower-level attachment function */
	void *ls_busdata;
	uint32_t ls_buspos;

	/* Initialized by higher-level attachment function */
	void *ls_devdata;
	void (*ls_start)(void *devdata);
	void (*ls_input)(void *devdata, int ch);
};

/* Functions called by lower-level drivers */
void lser_irq(/*struct lser_softc*/ void *sc);

/* Functions called by higher-level drivers */
void lser_write(/*struct lser_softc*/ void *sc, int ch);
void lser_writepolled(/*struct lser_softc*/ void *sc, int ch);

#endif /* _LAMEBUS_LSER_H_ */
