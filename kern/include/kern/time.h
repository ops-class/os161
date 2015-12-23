/*
 * Copyright (c) 2004, 2008
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

#ifndef _KERN_TIME_H_
#define _KERN_TIME_H_

/*
 * Time-related definitions, for <sys/time.h> and others.
 */


/*
 * Time with fractional seconds. Important. Unfortunately, to be
 * compatible, we need both timeval and timespec.
 */

struct timeval {
        __time_t tv_sec;        /* seconds */
        __i32 tv_usec;          /* microseconds */
};

struct timespec {
        __time_t tv_sec;        /* seconds */
        __i32 tv_nsec;          /* nanoseconds */
};


/*
 * Bits for interval timers. Obscure and not really that important.
 */

/* codes for the various timers */
#define ITIMER_REAL	0	/* Real (wall-clock) time. */
#define ITIMER_VIRTUAL	1	/* Virtual (when process is executing) time. */
#define ITIMER_PROF	2	/* For execution profiling. */

/* structure for setitimer/getitimer */
struct itimerval {
        struct timeval it_interval;	/* Time to reload after expiry. */
	struct timeval it_value;	/* Time to count. */
};


#endif /* _KERN_TIME_H_ */
