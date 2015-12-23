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
#include <lib.h>
#include <thread.h>
#include <threadprivate.h>

#include "switchframe.h"

/* in threadstart.S */
extern void mips_threadstart(/* arguments are in unusual registers */);


/*
 * Function to initialize the switchframe of a new thread, which is
 * *not* the one that is currently running.
 *
 * The new thread should, when it is run the first time, end up calling
 * thread_startup(entrypoint, data1, data2).
 *
 * We arrange for this by creating a phony switchframe for
 * switchframe_switch() to switch to. The only trouble is that the
 * switchframe doesn't include the argument registers a0-a3. So we
 * store the arguments in the s* registers, and use a bit of asm
 * (mips_threadstart) to move them and then jump to thread_startup.
 */
void
switchframe_init(struct thread *thread,
		 void (*entrypoint)(void *data1, unsigned long data2),
		 void *data1, unsigned long data2)
{
	vaddr_t stacktop;
	struct switchframe *sf;

        /*
         * MIPS stacks grow down. t_stack is just a hunk of memory, so
         * get the other end of it. Then set up a switchframe on the
         * top of the stack.
         */
        stacktop = ((vaddr_t)thread->t_stack) + STACK_SIZE;
        sf = ((struct switchframe *) stacktop) - 1;

        /* Zero out the switchframe. */
        bzero(sf, sizeof(*sf));

        /*
         * Now set the important parts: pass through the three arguments,
         * and set the return address register to the place we want
         * execution to begin.
         *
         * Thus, when switchframe_switch does its "j ra", it will
         * actually jump to mips_threadstart, which will move the
         * arguments into the right register and jump to
         * thread_startup().
         *
         * Note that this means that when we call switchframe_switch()
         * in thread_switch(), we may not come back out the same way
         * in the next thread. (Though we will come back out the same
         * way when we later come back to the same thread again.)
         *
         * This has implications for code at the bottom of
         * thread_switch, described in thread.c.
         */
        sf->sf_s0 = (uint32_t)entrypoint;
        sf->sf_s1 = (uint32_t)data1;
        sf->sf_s2 = (uint32_t)data2;
        sf->sf_ra = (uint32_t)mips_threadstart;

        /* Set ->t_context, and we're done. */
	thread->t_context = sf;
}
