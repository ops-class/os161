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
 * Driver for full-screen console.
 *
 * As of this writing the full-screen console is not supported in
 * System/161, so this driver is untested and probably broken.
 */
#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <platform/bus.h>
#include <lamebus/lscreen.h>
#include "autoconf.h"

/* Registers (offsets within slot) */
#define LSCR_REG_POSN    0   /* Cursor position */
#define LSCR_REG_SIZE    4   /* Display size */
#define LSCR_REG_CHAR    8   /* Character in */
#define LSCR_REG_RIRQ    12  /* Read interrupt status */

/* Bits in the IRQ registers */
#define LSCR_IRQ_ENABLE  1
#define LSCR_IRQ_ACTIVE  2

/* Offset within slot of screen buffer */
#define LSCR_SCREEN      32768

/* Convert a 32-bit X/Y pair to X and Y coordinates. */
static
inline
void
splitxy(uint32_t xy, unsigned *x, unsigned *y)
{
	*x = xy >> 16;
	*y = xy & 0xffff;
}

/* Convert X and Y coordinates to a single 32-bit value. */
static
inline
uint32_t
mergexy(unsigned x, unsigned y)
{
	uint32_t val = x;

	return (val << 16) | y;
}

////////////////////////////////////////////////////////////

/*
 * Interrupt handler.
 */
void
lscreen_irq(void *vsc)
{
	struct lscreen_softc *sc = vsc;
	uint32_t ch, x;

	spinlock_acquire(&sc->ls_lock);

	x = bus_read_register(sc->ls_busdata, sc->ls_buspos, LSCR_REG_RIRQ);
	if (x & LSCR_IRQ_ACTIVE) {
		ch = bus_read_register(sc->ls_busdata, sc->ls_buspos,
				       LSCR_REG_CHAR);
		bus_write_register(sc->ls_busdata, sc->ls_buspos,
				   LSCR_REG_RIRQ, LSCR_IRQ_ENABLE);

		spinlock_release(&sc->ls_lock);
		if (sc->ls_input) {
			sc->ls_input(sc->ls_devdata, ch);
		}
	}
	else {
		spinlock_release(&sc->ls_lock);
	}
}

////////////////////////////////////////////////////////////

/*
 * Handle a newline on the screen.
 */
static
void
lscreen_newline(struct lscreen_softc *sc)
{
	if (sc->ls_cy >= sc->ls_height-1) {
		/*
		 * Scroll
		 */

		memmove(sc->ls_screen, sc->ls_screen + sc->ls_width,
			sc->ls_width * (sc->ls_height-1));
		bzero(sc->ls_screen + sc->ls_width * (sc->ls_height-1),
		      sc->ls_width);
	}
	else {
		sc->ls_cy++;
	}
	sc->ls_cx=0;
}

/*
 * Handle a printable character being written to the screen.
 */
static
void
lscreen_char(struct lscreen_softc *sc, int ch)
{
	if (sc->ls_cx >= sc->ls_width) {
		lscreen_newline(sc);
	}

	sc->ls_screen[sc->ls_cy*sc->ls_width + sc->ls_cx] = ch;
	sc->ls_cx++;
}

/*
 * Send a character to the screen.
 * This should probably know about backspace and tab.
 */
void
lscreen_write(void *vsc, int ch)
{
	struct lscreen_softc *sc = vsc;
	int ccx, ccy;

	spinlock_acquire(&sc->ls_lock);

	switch (ch) {
	    case '\n': lscreen_newline(sc); break;
	    default: lscreen_char(sc, ch); break;
	}

	/*
	 * ccx/ccy = corrected cursor position
	 * (The cursor marks the next space text will appear in. But
	 * at the very end of the line, it should not move off the edge.)
	 */
	ccx = sc->ls_cx;
	ccy = sc->ls_cy;
	if (ccx==sc->ls_width) {
		ccx--;
	}

	/* Set the cursor position */
	bus_write_register(sc->ls_busdata, sc->ls_buspos,
			   LSCR_REG_POSN, mergexy(ccx, ccy));

	spinlock_release(&sc->ls_lock);
}

////////////////////////////////////////////////////////////

/*
 * Setup routine called by autoconf.c when an lscreen is found.
 */
int
config_lscreen(struct lscreen_softc *sc, int lscreenno)
{
	uint32_t val;

	(void)lscreenno;

	spinlock_init(&sc->ls_lock);

	/*
	 * Enable interrupting.
	 */

	bus_write_register(sc->ls_busdata, sc->ls_buspos,
			   LSCR_REG_RIRQ, LSCR_IRQ_ENABLE);

	/*
	 * Get screen size.
	 */
	val = bus_read_register(sc->ls_busdata, sc->ls_buspos,
				LSCR_REG_SIZE);
	splitxy(val, &sc->ls_width, &sc->ls_height);

	/*
	 * Get cursor position.
	 */
	val = bus_read_register(sc->ls_busdata, sc->ls_buspos,
				LSCR_REG_POSN);
	splitxy(val, &sc->ls_cx, &sc->ls_cy);

	/*
	 * Get a pointer to the memory-mapped screen area.
	 */
	sc->ls_screen = bus_map_area(sc->ls_busdata, sc->ls_buspos,
				     LSCR_SCREEN);

	return 0;
}

