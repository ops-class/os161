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
 * Machine-independent generic clock "device".
 *
 * Basically, all we do is remember something that can be used for
 * handling requests for the current time, and provide the gettime()
 * function to the rest of the kernel.
 *
 * The kernel config mechanism can be used to explicitly choose which
 * of the available clocks to use, if more than one is available.
 *
 * The system will panic if gettime() is called and there is no clock.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <clock.h>
#include <generic/rtclock.h>
#include "autoconf.h"

static struct rtclock_softc *the_clock = NULL;

int
config_rtclock(struct rtclock_softc *rtc, int unit)
{
	/* We use only the first clock device. */
	if (unit!=0) {
		return ENODEV;
	}

	KASSERT(the_clock==NULL);
	the_clock = rtc;
	return 0;
}

void
gettime(struct timespec *ts)
{
	KASSERT(the_clock!=NULL);
	the_clock->rtc_gettime(the_clock->rtc_devdata, ts);
}
