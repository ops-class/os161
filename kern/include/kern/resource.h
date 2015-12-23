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

#ifndef _KERN_RESOURCE_H_
#define _KERN_RESOURCE_H_

/*
 * Definitions for resource usage and limits.
 *
 * Not very important.
 */


/* priorities for setpriority() */
#define PRIO_MIN	(-20)
#define PRIO_MAX	20

/* "which" codes for setpriority() */
#define PRIO_PROCESS	0
#define PRIO_PGRP	1
#define PRIO_USER	2

/* flags for getrusage() */
#define RUSAGE_SELF	0
#define RUSAGE_CHILDREN	(-1)

struct rusage {
	struct timeval ru_utime;
	struct timeval ru_stime;
	__size_t ru_maxrss;		/* maximum RSS during lifespan (kb) */
	__counter_t ru_ixrss;		/* text memory usage (kb-ticks) */
	__counter_t ru_idrss;		/* data memory usage (kb-ticks) */
	__counter_t ru_isrss;		/* stack memory usage (kb-ticks) */
	__counter_t ru_minflt;		/* minor VM faults (count) */
	__counter_t ru_majflt;		/* major VM faults (count) */
	__counter_t ru_nswap;		/* whole-process swaps (count) */
	__counter_t ru_inblock;		/* file blocks read (count) */
	__counter_t ru_oublock;		/* file blocks written (count) */
	__counter_t ru_msgrcv;		/* socket/pipe packets rcv'd (count) */
	__counter_t ru_msgsnd;		/* socket/pipe packets sent (count) */
	__counter_t ru_nsignals;	/* signals delivered (count) */
	__counter_t ru_nvcsw;		/* voluntary context switches (count)*/
	__counter_t ru_nivcsw;		/* involuntary ditto (count) */
};

/* limit codes for getrusage/setrusage */

#define RLIMIT_NPROC		0	/* max procs per user (count) */
#define RLIMIT_NOFILE		1	/* max open files per proc (count) */
#define RLIMIT_CPU		2	/* cpu usage (seconds) */
#define RLIMIT_DATA		3	/* max .data/sbrk size (bytes) */
#define RLIMIT_STACK		4	/* max stack size (bytes) */
#define RLIMIT_MEMLOCK		5	/* max locked memory region (bytes) */
#define RLIMIT_RSS		6	/* max RSS (bytes) */
#define RLIMIT_CORE		7	/* core file size (bytes) */
#define RLIMIT_FSIZE		8	/* max file size (bytes) */
#define __RLIMIT_NUM		9	/* number of limits */

struct rlimit {
	__rlim_t rlim_cur;	/* soft limit */
	__rlim_t rlim_max;	/* hard limit */
};

#define RLIM_INFINITY	(~(__rlim_t)0)

#endif /* _KERN_RESOURCE_H_ */
