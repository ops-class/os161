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

#ifndef _MAINBUS_H_
#define _MAINBUS_H_

/*
 * Abstract system bus interface.
 */


struct cpu;       /* from <cpu.h> */
struct trapframe; /* from <machine/trapframe.h> */


/* Initialize the system bus and probe and attach hardware devices. */
void mainbus_bootstrap(void);

/* Start up secondary CPUs, once their cpu structures are set up */
void mainbus_start_cpus(void);

/* Bus-level interrupt handler, called from cpu-level trap/interrupt code */
void mainbus_interrupt(struct trapframe *);

/* Find the size of main memory. */
/* XXX this interface is not adequately MI */
size_t mainbus_ramsize(void);

/* Switch on an inter-processor interrupt. (Low-level.) */
void mainbus_send_ipi(struct cpu *target);

/*
 * The various ways to shut down the system. (These are very low-level
 * and should generally not be called directly - md_poweroff, for
 * instance, unceremoniously turns the power off without doing
 * anything else.)
 */
void mainbus_halt(void);
void mainbus_poweroff(void);
void mainbus_reboot(void);
void mainbus_panic(void);


#endif /* _MAINBUS_H_ */
