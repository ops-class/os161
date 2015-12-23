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

#ifndef _LAMEBUS_LSCREEN_H_
#define _LAMEBUS_LSCREEN_H_

/*
 * Hardware device data for memory-mapped fullscreen text console.
 */
struct lscreen_softc {
	/* Initialized by config function */
	struct spinlock ls_lock;      // protects data and device regs
	unsigned ls_width, ls_height; // screen size
	unsigned ls_cx, ls_cy;        // cursor position
	char *ls_screen;              // memory-mapped screen buffer

	/* Initialized by lower-level attachment function */
	void *ls_busdata;		// bus we're on
	uint32_t ls_buspos;		// position on that bus

	/* Initialized by higher-level attachment function */
	void *ls_devdata;			// data and functions for
	void (*ls_start)(void *devdata);	// upper device (perhaps
	void (*ls_input)(void *devdata, int ch); // console)
};

/* Functions called by lower-level drivers */
void lscreen_irq(/*struct lser_softc*/ void *sc);  // interrupt handler

/* Functions called by higher-level drivers */
void lscreen_write(/*struct lser_softc*/ void *sc, int ch); // output function

#endif /* _LAMEBUS_LSCREEN_H_ */
