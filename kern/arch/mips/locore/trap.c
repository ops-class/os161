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
#include <signal.h>
#include <lib.h>
#include <mips/specialreg.h>
#include <mips/trapframe.h>
#include <cpu.h>
#include <spl.h>
#include <thread.h>
#include <current.h>
#include <vm.h>
#include <mainbus.h>
#include <syscall.h>


/* in exception-*.S */
extern __DEAD void asm_usermode(struct trapframe *tf);

/* called only from assembler, so not declared in a header */
void mips_trap(struct trapframe *tf);


/* Names for trap codes */
#define NTRAPCODES 13
static const char *const trapcodenames[NTRAPCODES] = {
	"Interrupt",
	"TLB modify trap",
	"TLB miss on load",
	"TLB miss on store",
	"Address error on load",
	"Address error on store",
	"Bus error on code",
	"Bus error on data",
	"System call",
	"Break instruction",
	"Illegal instruction",
	"Coprocessor unusable",
	"Arithmetic overflow",
};

/*
 * Function called when user-level code hits a fatal fault.
 */
static
void
kill_curthread(vaddr_t epc, unsigned code, vaddr_t vaddr)
{
	int sig = 0;

	KASSERT(code < NTRAPCODES);
	switch (code) {
	    case EX_IRQ:
	    case EX_IBE:
	    case EX_DBE:
	    case EX_SYS:
		/* should not be seen */
		KASSERT(0);
		sig = SIGABRT;
		break;
	    case EX_MOD:
	    case EX_TLBL:
	    case EX_TLBS:
		sig = SIGSEGV;
		break;
	    case EX_ADEL:
	    case EX_ADES:
		sig = SIGBUS;
		break;
	    case EX_BP:
		sig = SIGTRAP;
		break;
	    case EX_RI:
		sig = SIGILL;
		break;
	    case EX_CPU:
		sig = SIGSEGV;
		break;
	    case EX_OVF:
		sig = SIGFPE;
		break;
	}

	/*
	 * You will probably want to change this.
	 */

	kprintf("Fatal user mode trap %u sig %d (%s, epc 0x%x, vaddr 0x%x)\n",
		code, sig, trapcodenames[code], epc, vaddr);
	panic("I don't know how to handle this\n");
}

/*
 * General trap (exception) handling function for mips.
 * This is called by the assembly-language exception handler once
 * the trapframe has been set up.
 */
void
mips_trap(struct trapframe *tf)
{
	uint32_t code;
	/*bool isutlb; -- not used */
	bool iskern;
	int spl;

	/* The trap frame is supposed to be 35 registers long. */
	KASSERT(sizeof(struct trapframe)==(35*4));

	/*
	 * Extract the exception code info from the register fields.
	 */
	code = (tf->tf_cause & CCA_CODE) >> CCA_CODESHIFT;
	/*isutlb = (tf->tf_cause & CCA_UTLB) != 0;*/
	iskern = (tf->tf_status & CST_KUp) == 0;

	KASSERT(code < NTRAPCODES);

	/* Make sure we haven't run off our stack */
	if (curthread != NULL && curthread->t_stack != NULL) {
		KASSERT((vaddr_t)tf > (vaddr_t)curthread->t_stack);
		KASSERT((vaddr_t)tf < (vaddr_t)(curthread->t_stack
						+ STACK_SIZE));
	}

	/* Interrupt? Call the interrupt handler and return. */
	if (code == EX_IRQ) {
		int old_in;
		bool doadjust;

		old_in = curthread->t_in_interrupt;
		curthread->t_in_interrupt = 1;

		/*
		 * The processor has turned interrupts off; if the
		 * currently recorded interrupt state is interrupts on
		 * (spl of 0), adjust the recorded state to match, and
		 * restore after processing the interrupt.
		 *
		 * How can we get an interrupt if the recorded state
		 * is interrupts off? Well, as things currently stand
		 * when the CPU finishes idling it flips interrupts on
		 * and off to allow things to happen, but leaves
		 * curspl high while doing so.
		 *
		 * While we're here, assert that the interrupt
		 * handling code hasn't leaked a spinlock or an
		 * splhigh().
		 */

		if (curthread->t_curspl == 0) {
			KASSERT(curthread->t_curspl == 0);
			KASSERT(curthread->t_iplhigh_count == 0);
			curthread->t_curspl = IPL_HIGH;
			curthread->t_iplhigh_count++;
			doadjust = true;
		}
		else {
			doadjust = false;
		}

		mainbus_interrupt(tf);

		if (doadjust) {
			KASSERT(curthread->t_curspl == IPL_HIGH);
			KASSERT(curthread->t_iplhigh_count == 1);
			curthread->t_iplhigh_count--;
			curthread->t_curspl = 0;
		}

		curthread->t_in_interrupt = old_in;
		goto done2;
	}

	/*
	 * The processor turned interrupts off when it took the trap.
	 *
	 * While we're in the kernel, and not actually handling an
	 * interrupt, restore the interrupt state to where it was in
	 * the previous context, which may be low (interrupts on).
	 *
	 * Do this by forcing splhigh(), which may do a redundant
	 * cpu_irqoff() but forces the stored MI interrupt state into
	 * sync, then restoring the previous state.
	 */
	spl = splhigh();
	splx(spl);

	/* Syscall? Call the syscall handler and return. */
	if (code == EX_SYS) {
		/* Interrupts should have been on while in user mode. */
		KASSERT(curthread->t_curspl == 0);
		KASSERT(curthread->t_iplhigh_count == 0);

		DEBUG(DB_SYSCALL, "syscall: #%d, args %x %x %x %x\n",
		      tf->tf_v0, tf->tf_a0, tf->tf_a1, tf->tf_a2, tf->tf_a3);

		syscall(tf);
		goto done;
	}

	/*
	 * Ok, it wasn't any of the really easy cases.
	 * Call vm_fault on the TLB exceptions.
	 * Panic on the bus error exceptions.
	 */
	switch (code) {
	case EX_MOD:
		if (vm_fault(VM_FAULT_READONLY, tf->tf_vaddr)==0) {
			goto done;
		}
		break;
	case EX_TLBL:
		if (vm_fault(VM_FAULT_READ, tf->tf_vaddr)==0) {
			goto done;
		}
		break;
	case EX_TLBS:
		if (vm_fault(VM_FAULT_WRITE, tf->tf_vaddr)==0) {
			goto done;
		}
		break;
	case EX_IBE:
	case EX_DBE:
		/*
		 * This means you loaded invalid TLB entries, or
		 * touched invalid parts of the direct-mapped
		 * segments. These are serious kernel errors, so
		 * panic.
		 *
		 * The MIPS won't even tell you what invalid address
		 * caused the bus error.
		 */
		panic("Bus error exception, PC=0x%x\n", tf->tf_epc);
		break;
	}

	/*
	 * If we get to this point, it's a fatal fault - either it's
	 * one of the other exceptions, like illegal instruction, or
	 * it was a page fault we couldn't handle.
	 */

	if (!iskern) {
		/*
		 * Fatal fault in user mode.
		 * Kill the current user process.
		 */
		kill_curthread(tf->tf_epc, code, tf->tf_vaddr);
		goto done;
	}

	/*
	 * Fatal fault in kernel mode.
	 *
	 * If pcb_badfaultfunc is set, we do not panic; badfaultfunc is
	 * set by copyin/copyout and related functions to signify that
	 * the addresses they're accessing are userlevel-supplied and
	 * not trustable. What we actually want to do is resume
	 * execution at the function pointed to by badfaultfunc. That's
	 * going to be "copyfail" (see copyinout.c), which longjmps
	 * back to copyin/copyout or wherever and returns EFAULT.
	 *
	 * Note that we do not just *call* this function, because that
	 * won't necessarily do anything. We want the control flow
	 * that is currently executing in copyin (or whichever), and
	 * is stopped while we process the exception, to *teleport* to
	 * copyfail.
	 *
	 * This is accomplished by changing tf->tf_epc and returning
	 * from the exception handler.
	 */

	if (curthread != NULL &&
	    curthread->t_machdep.tm_badfaultfunc != NULL) {
		tf->tf_epc = (vaddr_t) curthread->t_machdep.tm_badfaultfunc;
		goto done;
	}

	/*
	 * Really fatal kernel-mode fault.
	 */

	kprintf("panic: Fatal exception %u (%s) in kernel mode\n", code,
		trapcodenames[code]);
	kprintf("panic: EPC 0x%x, exception vaddr 0x%x\n",
		tf->tf_epc, tf->tf_vaddr);

	panic("I can't handle this... I think I'll just die now...\n");

 done:
	/*
	 * Turn interrupts off on the processor, without affecting the
	 * stored interrupt state.
	 */
	cpu_irqoff();
 done2:

	/*
	 * The boot thread can get here (e.g. on interrupt return) but
	 * since it doesn't go to userlevel, it can't be returning to
	 * userlevel, so there's no need to set cputhreads[] and
	 * cpustacks[]. Just return.
	 */
	if (curthread->t_stack == NULL) {
		return;
	}

	cputhreads[curcpu->c_number] = (vaddr_t)curthread;
	cpustacks[curcpu->c_number] = (vaddr_t)curthread->t_stack + STACK_SIZE;

	/*
	 * This assertion will fail if either
	 *   (1) curthread->t_stack is corrupted, or
	 *   (2) the trap frame is somehow on the wrong kernel stack.
	 *
	 * If cpustacks[] is corrupted, the next trap back to the
	 * kernel will (most likely) hang the system, so it's better
	 * to find out now.
	 */
	KASSERT(SAME_STACK(cpustacks[curcpu->c_number]-1, (vaddr_t)tf));
}

/*
 * Function for entering user mode.
 *
 * This should not be used by threads returning from traps - they
 * should just return from mips_trap(). It should be used by threads
 * entering user mode for the first time - whether the child thread in
 * a fork(), or into a brand-new address space after exec(), or when
 * starting the first userlevel program.
 *
 * It works by jumping into the exception return code.
 *
 * mips_usermode is common code for this. It cannot usefully be called
 * outside the mips port, but should be called from one of the
 * following places:
 *    - enter_new_process, for use by exec and equivalent.
 *    - enter_forked_process, in syscall.c, for use by fork.
 */
void
mips_usermode(struct trapframe *tf)
{

	/*
	 * Interrupts should be off within the kernel while entering
	 * user mode. However, while in user mode, interrupts should
	 * be on. To interact properly with the spl-handling logic
	 * above, we explicitly call spl0() and then call cpu_irqoff().
	 */
	spl0();
	cpu_irqoff();

	cputhreads[curcpu->c_number] = (vaddr_t)curthread;
	cpustacks[curcpu->c_number] = (vaddr_t)curthread->t_stack + STACK_SIZE;

	/*
	 * This assertion will fail if either
	 *   (1) cpustacks[] is corrupted, or
	 *   (2) the trap frame is not on our own kernel stack, or
	 *   (3) the boot thread tries to enter user mode.
	 *
	 * If cpustacks[] is corrupted, the next trap back to the
	 * kernel will (most likely) hang the system, so it's better
	 * to find out now.
	 *
	 * It's necessary for the trap frame used here to be on the
	 * current thread's own stack. It cannot correctly be on
	 * either another thread's stack or in the kernel heap.
	 * (Exercise: why?)
	 */
	KASSERT(SAME_STACK(cpustacks[curcpu->c_number]-1, (vaddr_t)tf));

	/*
	 * This actually does it. See exception-*.S.
	 */
	asm_usermode(tf);
}

/*
 * enter_new_process: go to user mode after loading an executable.
 *
 * Performs the necessary initialization so that the user program will
 * get the arguments supplied in argc/argv (note that argv must be a
 * user-level address) and the environment pointer env (ditto), and
 * begin executing at the specified entry point. The stack pointer is
 * initialized from the stackptr argument. Note that passing argc/argv
 * may use additional stack space on some other platforms (but not on
 * mips).
 *
 * Unless you implement execve() that passes environments around, just
 * pass NULL for the environment.
 *
 * Works by creating an ersatz trapframe.
 */
void
enter_new_process(int argc, userptr_t argv, userptr_t env,
		  vaddr_t stack, vaddr_t entry)
{
	struct trapframe tf;

	bzero(&tf, sizeof(tf));

	tf.tf_status = CST_IRQMASK | CST_IEp | CST_KUp;
	tf.tf_epc = entry;
	tf.tf_a0 = argc;
	tf.tf_a1 = (vaddr_t)argv;
	tf.tf_a2 = (vaddr_t)env;
	tf.tf_sp = stack;

	mips_usermode(&tf);
}
