/*
 * Copyright (c) 2003, 2008
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

#ifndef _KERN_WAIT_H_
#define _KERN_WAIT_H_

/*
 * Definitions for wait().
 */


/* Flags for waitpid() and equivalent. */
#define WNOHANG      1	/* Nonblocking. */
#define WUNTRACED    2	/* Report stopping as well as exiting processes. */

/* Special "pids" to wait for. */
#define WAIT_ANY     (-1)	/* Any child process. */
#define WAIT_MYPGRP  0		/* Any process in the same process group. */

/*
 * Result encoding.
 *
 * The lowest two bits say what happened; the rest encodes up to 30
 * bits of exit code. Note that the traditional Unix encoding, which
 * is different, wastes most of the bits and can only transmit 8 bits
 * of exit code...
 */
#define _WWHAT(x)  ((x)&3)	/* lower two bits say what happened */
#define _WVAL(x)   ((x)>>2)	/* the rest is the value */
#define _MKWVAL(x) ((x)<<2)	/* encode a value */

/* Four things can happen... */
#define __WEXITED    0		/* Process exited by calling _exit(). */
#define __WSIGNALED  1		/* Process received a fatal signal. */
#define __WCORED     2		/* Process dumped core on a fatal signal. */
#define __WSTOPPED   3		/* Process stopped (and didn't exit). */

/* Test macros, used by applications. */
#define WIFEXITED(x)   (_WWHAT(x)==__WEXITED)
#define WIFSIGNALED(x) (_WWHAT(x)==__WSIGNALED || _WWHAT(x)==__WCORED)
#define WIFSTOPPED(x)  (_WWHAT(x)==__WSTOPPED)
#define WEXITSTATUS(x) (_WVAL(x))
#define WTERMSIG(x)    (_WVAL(x))
#define WSTOPSIG(x)    (_WVAL(x))
#define WCOREDUMP(x)   (_WWHAT(x)==__WCORED)

/* Encoding macros, used by the kernel to generate the wait result. */
#define _MKWAIT_EXIT(x) (_MKWVAL(x)|__WEXITED)
#define _MKWAIT_SIG(x)  (_MKWVAL(x)|__WSIGNALED)
#define _MKWAIT_CORE(x) (_MKWVAL(x)|__WCORED)
#define _MKWAIT_STOP(x) (_MKWVAL(x)|__WSTOPPED)

#endif /* _KERN_WAIT_H_ */
