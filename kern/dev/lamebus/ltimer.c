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
 * Driver for LAMEbus clock/timer card
 */
#include <types.h>
#include <lib.h>
#include <spl.h>
#include <clock.h>
#include <platform/bus.h>
#include <lamebus/ltimer.h>
#include "autoconf.h"

/* Registers (offsets within slot) */
#define LT_REG_SEC    0     /* time of day: seconds */
#define LT_REG_NSEC   4     /* time of day: nanoseconds */
#define LT_REG_ROE    8     /* Restart On countdown-timer Expiry flag */
#define LT_REG_IRQ    12    /* Interrupt status register */
#define LT_REG_COUNT  16    /* Time for countdown timer (usec) */
#define LT_REG_SPKR   20    /* Beep control */

/* Granularity of countdown timer (usec) */
#define LT_GRANULARITY   1000000

static bool havetimerclock;

/*
 * Setup routine called by autoconf stuff when an ltimer is found.
 */
int
config_ltimer(struct ltimer_softc *lt, int ltimerno)
{
	/*
	 * Running on System/161 2.x, we always use the processor
	 * on-chip timer for hardclock and we don't need ltimer as
	 * hardclock.
	 *
	 * Ideally there should be code here that will use an ltimer
	 * for hardclock if nothing else is available; e.g. if we
	 * wanted to make OS/161 2.x run on System/161 1.x. However,
	 * that requires a good bit more infrastructure for handling
	 * timers than we have and it doesn't seem worthwhile.
	 *
	 * It would also require some hacking, because all CPUs need
	 * to receive timer interrupts. (Exercise: how would you make
	 * sure all CPUs receive exactly one timer interrupt? Remember
	 * that LAMEbus uses level-triggered interrupts, so the
	 * hardware interrupt line will cause repeated interrupts if
	 * it's not reset on the device; but if it's reset on the
	 * device before all CPUs manage to see it, those CPUs won't
	 * be interrupted at all.)
	 *
	 * Note that the beep and rtclock devices *do* attach to
	 * ltimer.
	 */
	(void)ltimerno;
	lt->lt_hardclock = 0;

	/*
	 * We do, however, use ltimer for the timer clock, since the
	 * on-chip timer can't do that.
	 */
	if (!havetimerclock) {
		havetimerclock = true;
		lt->lt_timerclock = 1;

		/* Wire it to go off once every second. */
		bus_write_register(lt->lt_bus, lt->lt_buspos, LT_REG_ROE, 1);
		bus_write_register(lt->lt_bus, lt->lt_buspos, LT_REG_COUNT,
				   LT_GRANULARITY);
	}

	return 0;
}

/*
 * Interrupt handler.
 */
void
ltimer_irq(void *vlt)
{
	struct ltimer_softc *lt = vlt;
	uint32_t val;

	val = bus_read_register(lt->lt_bus, lt->lt_buspos, LT_REG_IRQ);
	if (val) {
		/*
		 * Only call hardclock if we're responsible for hardclock.
		 * (Any additional timer devices are unused.)
		 */
		if (lt->lt_hardclock) {
			hardclock();
		}
		/*
		 * Likewise for timerclock.
		 */
		if (lt->lt_timerclock) {
			timerclock();
		}
	}
}

/*
 * The timer device will beep if you write to the beep register. It
 * doesn't matter what value you write. This function is called if
 * the beep device is attached to this timer.
 */
void
ltimer_beep(void *vlt)
{
	struct ltimer_softc *lt = vlt;

	bus_write_register(lt->lt_bus, lt->lt_buspos, LT_REG_SPKR, 440);
}

/*
 * The timer device also has a realtime clock on it.
 * This function gets called if the rtclock device is attached
 * to this timer.
 */
void
ltimer_gettime(void *vlt, struct timespec *ts)
{
	struct ltimer_softc *lt = vlt;
	uint32_t secs1, secs2;
	int spl;

	/*
	 * Read the seconds twice, on either side of the nanoseconds.
	 * If nsecs is small, use the *later* value of seconds, in case
	 * the nanoseconds turned over between the time we got the earlier
	 * value and the time we got nsecs.
	 *
	 * Note that the clock in the ltimer device is accurate down
	 * to a single processor cycle, so this might actually matter
	 * now and then.
	 *
	 * Do it with interrupts off on the current processor to avoid
	 * getting garbage if we get an interrupt among the register
	 * reads.
	 */

	spl = splhigh();

	secs1 = bus_read_register(lt->lt_bus, lt->lt_buspos,
				  LT_REG_SEC);
	ts->tv_nsec = bus_read_register(lt->lt_bus, lt->lt_buspos,
				   LT_REG_NSEC);
	secs2 = bus_read_register(lt->lt_bus, lt->lt_buspos,
				  LT_REG_SEC);

	splx(spl);

	if (ts->tv_nsec < 5000000) {
		ts->tv_sec = secs2;
	}
	else {
		ts->tv_sec = secs1;
	}
}
