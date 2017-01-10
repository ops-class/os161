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
#include <kern/unistd.h>
#include <stdarg.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <mainbus.h>
#include <vfs.h>          // for vfs_sync()
#include <lamebus/ltrace.h> // for ltrace_stop()
#include <kern/secret.h>
#include <test.h>


/* Flags word for DEBUG() macro. */
uint32_t dbflags = 0;

/* Lock for non-polled kprintfs */
static struct lock *kprintf_lock;

/* Lock for polled kprintfs */
static struct spinlock kprintf_spinlock;


/*
 * Warning: all this has to work from interrupt handlers and when
 * interrupts are disabled.
 */


/*
 * Create the kprintf lock. Must be called before creating a second
 * thread or enabling a second CPU.
 */
void
kprintf_bootstrap(void)
{
	KASSERT(kprintf_lock == NULL);

	kprintf_lock = lock_create("kprintf_lock");
	if (kprintf_lock == NULL) {
		panic("Could not create kprintf_lock\n");
	}
	spinlock_init(&kprintf_spinlock);
}

/*
 * Send characters to the console. Backend for __printf.
 */
static
void
console_send(void *junk, const char *data, size_t len)
{
	size_t i;

	(void)junk;

	for (i=0; i<len; i++) {
		putch(data[i]);
	}
}

/*
 * kprintf and tprintf helper function.
 */
static
inline
int
__kprintf(const char *fmt, va_list ap)
{
	int chars;
	bool dolock;

	dolock = kprintf_lock != NULL
		&& curthread->t_in_interrupt == false
		&& curthread->t_curspl == 0
		&& curcpu->c_spinlocks == 0;

	if (dolock) {
		lock_acquire(kprintf_lock);
	}
	else {
		spinlock_acquire(&kprintf_spinlock);
	}

	chars = __vprintf(console_send, NULL, fmt, ap);

	if (dolock) {
		lock_release(kprintf_lock);
	}
	else {
		spinlock_release(&kprintf_spinlock);
	}

	return chars;
}

/*
 * Printf to the console.
 */
int
kprintf(const char *fmt, ...)
{
	int chars;
	va_list ap;

	va_start(ap, fmt);
	chars = __kprintf(fmt, ap);
	va_end(ap);

	return chars;
}

/*
 * panic() is for fatal errors. It prints the printf arguments it's
 * passed and then halts the system.
 */

void
panic(const char *fmt, ...)
{
	va_list ap;

	/*
	 * When we reach panic, the system is usually fairly screwed up.
	 * It's not entirely uncommon for anything else we try to do
	 * here to trigger more panics.
	 *
	 * This variable makes sure that if we try to do something here,
	 * and it causes another panic, *that* panic doesn't try again;
	 * trying again almost inevitably causes infinite recursion.
	 *
	 * This is not excessively paranoid - these things DO happen!
	 */
	static volatile int evil;

	if (evil == 0) {
		evil = 1;

		/*
		 * Not only do we not want to be interrupted while
		 * panicking, but we also want the console to be
		 * printing in polling mode so as not to do context
		 * switches. So turn interrupts off on this CPU.
		 */
		splhigh();
	}

	if (evil == 1) {
		evil = 2;

		/* Kill off other threads and halt other CPUs. */
		thread_panic();
	}

	if (evil == 2) {
		evil = 3;

		/* Print the message. */
		kprintf("panic: ");
		va_start(ap, fmt);
		__vprintf(console_send, NULL, fmt, ap);
		va_end(ap);
	}

	if (evil == 3) {
		evil = 4;

		/* Drop to the debugger. */
		ltrace_stop(0);
	}

	if (evil == 4) {
		evil = 5;

		/* Try to sync the disks. */
		vfs_sync();
	}

	if (evil == 5) {
		evil = 6;

		/* Shut down or reboot the system. */
		mainbus_panic();
	}

	/*
	 * Last resort, just in case.
	 */

	for (;;);
}

/*
 * Assertion failures go through this.
 */
void
badassert(const char *expr, const char *file, int line, const char *func)
{
	panic("Assertion failed: %s, at %s:%d (%s)\n",
	      expr, file, line, func);
}
