/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)signal.h	8.4 (Berkeley) 5/4/95
 */

#ifndef _KERN_SIGNAL_H_
#define _KERN_SIGNAL_H_

/*
 * Machine-independent definitions for signals.
 */


/*
 * The signals.
 *
 * The values of many of these are "well known", particularly 1, 9,
 * 10, and 11.
 *
 * Note that Unix signals are a semantic cesspool; many have special
 * properties or are supposed to interact with the system in special
 * ways. It is gross.
 */

#define SIGHUP		1	/* Hangup */
#define SIGINT		2	/* Interrupt (^C) */
#define SIGQUIT		3	/* Quit (typically ^\) */
#define SIGILL		4	/* Illegal instruction */
#define SIGTRAP		5	/* Breakpoint trap */
#define SIGABRT		6	/* abort() call */
#define SIGEMT		7	/* Emulator trap */
#define SIGFPE		8	/* Floating point exception */
#define SIGKILL		9	/* Hard kill (unblockable) */
#define SIGBUS		10	/* Bus error, typically bad pointer alignment*/
#define SIGSEGV		11	/* Segmentation fault */
#define SIGSYS		12	/* Bad system call */
#define SIGPIPE		13	/* Broken pipe */
#define SIGALRM		14	/* alarm() expired */
#define SIGTERM		15	/* Termination requested (default kill) */
#define SIGURG		16	/* Urgent data on socket */
#define SIGSTOP		17	/* Hard process stop (unblockable) */
#define SIGTSTP		18	/* Terminal stop (^Z) */
#define SIGCONT		19	/* Time to continue after stop */
#define SIGCHLD		20	/* Child process exited */
#define SIGTTIN		21	/* Stop on tty read while in background */
#define SIGTTOU		22	/* Stop on tty write while in background */
#define SIGIO		23	/* Nonblocking or async I/O is now ready */
#define SIGXCPU		24	/* CPU time resource limit exceeded */
#define SIGXFSZ		25	/* File size resource limit exceeded */
#define SIGVTALRM	26	/* Like SIGALRM but in virtual time */
#define SIGPROF		27	/* Profiling timer */
#define SIGWINCH	28	/* Window size change on tty */
#define SIGINFO		29	/* Information request (typically ^T) */
#define SIGUSR1		20	/* Application-defined */
#define SIGUSR2		31	/* Application-defined */
#define SIGPWR		32	/* Power failure */
#define _NSIG		32


/* Type for a set of signals; used by e.g. sigprocmask(). */
typedef __u32 sigset_t;

/* flags for sigaction.sa_flags */
#define SA_ONSTACK	1	/* Use sigaltstack() stack. */
#define SA_RESTART	2	/* Restart syscall instead of interrupting. */
#define SA_RESETHAND	4	/* Clear handler after one usage. */

/* codes for sigprocmask() */
#define SIG_BLOCK	1	/* Block selected signals. */
#define SIG_UNBLOCK	2	/* Unblock selected signals. */
#define SIG_SETMASK	3	/* Set mask to the selected signals. */

/* Type for a signal handler function. */
typedef void (*__sigfunc)(int);

/* Magic values for signal handlers. */
#define SIG_DFL		((__sigfunc) 0)		/* Default behavior. */
#define SIG_IGN		((__sigfunc) 1)		/* Ignore the signal. */

/*
 * Struct for sigaction().
 */
struct sigaction {
	__sigfunc sa_handler;
	sigset_t sa_mask;
	unsigned sa_flags;
};

/*
 * Struct for sigaltstack().
 * (not very important)
 */
struct sigaltstack {
	void *ss_sp;
	size_t ss_size;
	unsigned ss_flags;
};


#endif /* _KERN_SIGNAL_H_ */
