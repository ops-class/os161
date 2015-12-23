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

#ifndef _CLOCK_H_
#define _CLOCK_H_

/*
 * Time-related definitions.
 */

#include <kern/time.h>


/*
 * hardclock() is called on every CPU HZ times a second, possibly only
 * when the CPU is not idle, for scheduling.
 */

/* hardclocks per second */
#define HZ  100

void hardclock_bootstrap(void);
void hardclock(void);

/*
 * timerclock() is called on one CPU once a second to allow simple
 * timed operations. (This is a fairly simpleminded interface.)
 */
void timerclock(void);

/*
 * gettime() may be used to fetch the current time of day.
 */
void gettime(struct timespec *ret);

/*
 * arithmetic on times
 *
 * add: ret = t1 + t2
 * sub: ret = t1 - t2
 */

void timespec_add(const struct timespec *t1,
		  const struct timespec *t2,
		  struct timespec *ret);
void timespec_sub(const struct timespec *t1,
		  const struct timespec *t2,
		  struct timespec *ret);

/*
 * clocksleep() suspends execution for the requested number of seconds,
 * like userlevel sleep(3). (Don't confuse it with wchan_sleep.)
 */
void clocksleep(int seconds);


#endif /* _CLOCK_H_ */
