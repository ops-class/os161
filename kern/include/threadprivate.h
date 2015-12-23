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

#ifndef _THREADPRIVATE_H_
#define _THREADPRIVATE_H_

struct thread;		/* from <thread.h> */
struct thread_machdep;	/* from <machine/thread.h> */
struct switchframe;	/* from <machine/switchframe.h> */


/*
 * Subsystem-private thread defs.
 *
 * This file is to be used only by the thread subsystem. However, it
 * has to be placed in the public include directory (rather than the
 * threads directory) so the machine-dependent thread code can include
 * it. This is one of the drawbacks of putting all machine-dependent
 * material in a single directory: it exposes what ought to be private
 * interfaces.
 */


/*
 * Private thread functions.
 */

/* Entry point for new threads. */
void thread_startup(void (*entrypoint)(void *data1, unsigned long data2),
		    void *data1, unsigned long data2);

/* Initialize or clean up the machine-dependent portion of struct thread */
void thread_machdep_init(struct thread_machdep *tm);
void thread_machdep_cleanup(struct thread_machdep *tm);

/*
 * Machine-dependent functions for working on switchframes.
 *
 * Note that while the functions themselves are machine-dependent, their
 * declarations are not.
 */

/* Assembler-level context switch. */
void switchframe_switch(struct switchframe **prev, struct switchframe **next);

/* Thread initialization */
void switchframe_init(struct thread *,
		      void (*entrypoint)(void *data1, unsigned long data2),
		      void *data1, unsigned long data2);


#endif /* _THREADPRIVATE_H_ */
