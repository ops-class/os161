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
#include <clock.h>

/*
 * ts1 + ts2
 */
void
timespec_add(const struct timespec *ts1,
	     const struct timespec *ts2,
	     struct timespec *ret)
{
	ret->tv_nsec = ts1->tv_nsec + ts2->tv_nsec;
	ret->tv_sec = ts1->tv_sec + ts2->tv_sec;
	if (ret->tv_nsec >= 1000000000) {
		ret->tv_nsec -= 1000000000;
		ret->tv_sec += 1;
	}
}

/*
 * ts1 - ts2
 */
void
timespec_sub(const struct timespec *ts1,
	     const struct timespec *ts2,
	     struct timespec *ret)
{
	/* in case ret and ts1 or ts2 are the same */
	struct timespec r;

	r = *ts1;
	if (r.tv_nsec < ts2->tv_nsec) {
		r.tv_nsec += 1000000000;
		r.tv_sec--;
	}

	r.tv_nsec -= ts2->tv_nsec;
	r.tv_sec -= ts2->tv_sec;
	*ret = r;
}
