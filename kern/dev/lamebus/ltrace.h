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

#ifndef _LAMEBUS_LTRACE_H_
#define _LAMEBUS_LTRACE_H_

struct ltrace_softc {
	/* Initialized by lower-level attachment function */
	void *lt_busdata;
	uint32_t lt_buspos;
	bool lt_canstop;
	bool lt_canprof;
};

/*
 * Functions provided for debug hacking:
 *   ltrace_on:    turns on the trace161 tracing flag CODE.
 *   ltrace_off:   turns off the trace161 tracing flag CODE.
 *   ltrace_debug: causes sys161/trace161 to print a message with CODE.
 *   ltrace_dump:  causes trace161 to do a complete state dump, tagged CODE.
 *   ltrace_stop:  causes sys161/trace161 to drop to the debugger.
 *   ltrace_setprof: turn on and off trace161 profile collection.
 *   ltrace_eraseprof: discard trace161 profile collected so far.
 *
 * The flags for ltrace_on/off are the characters used to control
 * tracing on the trace161 command line. See the System/161 manual for
 * more information.
 *
 * ltrace_debug is for printing simple indications that a certain
 * piece of code has been reached, like one might use kprintf, except
 * that it is less invasive than kprintf. Think of it as setting the
 * value of a readout on the system's front panel. (In real life,
 * since computers don't have front panels with blinking lights any
 * more, people often use the speaker or the top left corner of the
 * screen for this purpose.)
 *
 * ltrace_dump dumps the entire system state and is primarily intended
 * for regression testing of System/161. It might or might not prove
 * useful for debugging as well.
 *
 * Calling ltrace_stop behaves similarly to hardwiring a breakpoint
 * instruction in your code, except that debuggers have trouble
 * stepping past hardwired breakpoints. Currently the value of the
 * code is ignored.
 *
 * ltrace_setprof can be used to dynamically turn profiling on and
 * off, if trace161 is collecting a profile. (Otherwise it does
 * nothing.) This can be used to e.g. profile only code that executes
 * while holding a given lock.
 *
 * ltrace_eraseprof can be used to clear the accumulated profile
 * data, if trace161 is collecting a profile. (Otherwise it does
 * nothing.) This can be used to e.g. exclude bootup actions from your
 * profile.
 */
void ltrace_on(uint32_t code);
void ltrace_off(uint32_t code);
void ltrace_debug(uint32_t code);
void ltrace_dump(uint32_t code);
void ltrace_stop(uint32_t code);
void ltrace_setprof(uint32_t onoff);
void ltrace_eraseprof(void);

#endif /* _LAMEBUS_LTRACE_H_ */
