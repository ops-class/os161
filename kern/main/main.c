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

/*
 * Main.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/reboot.h>
#include <kern/unistd.h>
#include <lib.h>
#include <spl.h>
#include <clock.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <vm.h>
#include <mainbus.h>
#include <vfs.h>
#include <device.h>
#include <syscall.h>
#include <test.h>
#include <version.h>
#include "autoconf.h"  // for pseudoconfig


/*
 * These two pieces of data are maintained by the makefiles and build system.
 * buildconfig is the name of the config file the kernel was configured with.
 * buildversion starts at 1 and is incremented every time you link a kernel.
 *
 * The purpose is not to show off how many kernels you've linked, but
 * to make it easy to make sure that the kernel you just booted is the
 * same one you just built.
 */
extern const int buildversion;
extern const char buildconfig[];

/*
 * Copyright message for the OS/161 base code.
 */
static const char harvard_copyright[] =
    "Copyright (c) 2000, 2001-2005, 2008-2011, 2013, 2014\n"
    "   President and Fellows of Harvard College.  All rights reserved.\n";


/*
 * Initial boot sequence.
 */
static
void
boot(void)
{
	/*
	 * The order of these is important!
	 * Don't go changing it without thinking about the consequences.
	 *
	 * Among other things, be aware that console output gets
	 * buffered up at first and does not actually appear until
	 * mainbus_bootstrap() attaches the console device. This can
	 * be remarkably confusing if a bug occurs at this point. So
	 * don't put new code before mainbus_bootstrap if you don't
	 * absolutely have to.
	 *
	 * Also note that the buffer for this is only 1k. If you
	 * overflow it, the system will crash without printing
	 * anything at all. You can make it larger though (it's in
	 * dev/generic/console.c).
	 */

	kprintf("\n");
	kprintf("OS/161 base system version %s\n", BASE_VERSION);
	kprintf("%s", harvard_copyright);
	kprintf("\n");

	kprintf("Put-your-group-name-here's system version %s (%s #%d)\n",
		GROUP_VERSION, buildconfig, buildversion);
	kprintf("\n");

	/* Early initialization. */
	ram_bootstrap();
	proc_bootstrap();
	thread_bootstrap();
	hardclock_bootstrap();
	vfs_bootstrap();
	kheap_nextgeneration();

	/* Probe and initialize devices. Interrupts should come on. */
	kprintf("Device probe...\n");
	KASSERT(curthread->t_curspl > 0);
	mainbus_bootstrap();
	KASSERT(curthread->t_curspl == 0);
	/* Now do pseudo-devices. */
	pseudoconfig();
	kprintf("\n");
	kheap_nextgeneration();

	/* Late phase of initialization. */
	vm_bootstrap();
	kprintf_bootstrap();
	thread_start_cpus();

	/* Default bootfs - but ignore failure, in case emu0 doesn't exist */
	vfs_setbootfs("emu0");

	kheap_nextgeneration();

	/*
	 * Make sure various things aren't screwed up.
	 */
	COMPILE_ASSERT(sizeof(userptr_t) == sizeof(char *));
	COMPILE_ASSERT(sizeof(*(userptr_t)0) == sizeof(char));
}

/*
 * Shutdown sequence. Opposite to boot().
 */
static
void
shutdown(void)
{

	kprintf("Shutting down.\n");

	vfs_clearbootfs();
	vfs_clearcurdir();
	vfs_unmountall();

	thread_shutdown();

	splhigh();
}

/*****************************************/

/*
 * reboot() system call.
 *
 * Note: this is here because it's directly related to the code above,
 * not because this is where system call code should go. Other syscall
 * code should probably live in the "syscall" directory.
 */
int
sys_reboot(int code)
{
	switch (code) {
	    case RB_REBOOT:
	    case RB_HALT:
	    case RB_POWEROFF:
		break;
	    default:
		return EINVAL;
	}

	shutdown();

	switch (code) {
	    case RB_HALT:
		kprintf("The system is halted.\n");
		mainbus_halt();
		break;
	    case RB_REBOOT:
		kprintf("Rebooting...\n");
		mainbus_reboot();
		break;
	    case RB_POWEROFF:
		kprintf("The system is halted.\n");
		mainbus_poweroff();
		break;
	}

	panic("reboot operation failed\n");
	return 0;
}

/*
 * Kernel main. Boot up, then fork the menu thread; wait for a reboot
 * request, and then shut down.
 */
void
kmain(char *arguments)
{
	boot();

	menu(arguments);

	/* Should not get here */
}
