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
 * Code for attaching the (generic) rtclock device to the LAMEbus ltimer.
 *
 * rtclock is a generic clock interface that gets its clock service from
 * an actual hardware clock of some kind. (Theoretically it could also
 * get its clock service from a clock maintained in software, as is the
 * case on most systems. However, no such driver has been written yet.)
 *
 * ltimer can provide this clock service.
 */

#include <types.h>
#include <lib.h>
#include <generic/rtclock.h>
#include <lamebus/ltimer.h>
#include "autoconf.h"

struct rtclock_softc *
attach_rtclock_to_ltimer(int rtclockno, struct ltimer_softc *ls)
{
	/*
	 * No need to probe; ltimer always has a clock.
	 * Just allocate the rtclock, set our fields, and return it.
	 */
	struct rtclock_softc *rtc = kmalloc(sizeof(struct rtclock_softc));
	if (rtc==NULL) {
		/* Out of memory */
		return NULL;
	}

	(void)rtclockno;  // unused

	rtc->rtc_devdata = ls;
	rtc->rtc_gettime = ltimer_gettime;

	return rtc;
}
