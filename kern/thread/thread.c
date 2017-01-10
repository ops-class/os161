/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009, 2010
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
 * Core kernel-level thread system.
 */

#define THREADINLINE

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <array.h>
#include <cpu.h>
#include <spl.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <threadlist.h>
#include <threadprivate.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <addrspace.h>
#include <mainbus.h>
#include <vnode.h>


/* Magic number used as a guard value on kernel thread stacks. */
#define THREAD_STACK_MAGIC 0xbaadf00d

/* Wait channel. A wchan is protected by an associated, passed-in spinlock. */
struct wchan {
	const char *wc_name;		/* name for this channel */
	struct threadlist wc_threads;	/* list of waiting threads */
};

/* Master array of CPUs. */
DECLARRAY(cpu, static __UNUSED inline);
DEFARRAY(cpu, static __UNUSED inline);
static struct cpuarray allcpus;

/* Used to wait for secondary CPUs to come online. */
static struct semaphore *cpu_startup_sem;

////////////////////////////////////////////////////////////

/*
 * Stick a magic number on the bottom end of the stack. This will
 * (sometimes) catch kernel stack overflows. Use thread_checkstack()
 * to test this.
 */
static
void
thread_checkstack_init(struct thread *thread)
{
	((uint32_t *)thread->t_stack)[0] = THREAD_STACK_MAGIC;
	((uint32_t *)thread->t_stack)[1] = THREAD_STACK_MAGIC;
	((uint32_t *)thread->t_stack)[2] = THREAD_STACK_MAGIC;
	((uint32_t *)thread->t_stack)[3] = THREAD_STACK_MAGIC;
}

/*
 * Check the magic number we put on the bottom end of the stack in
 * thread_checkstack_init. If these assertions go off, it most likely
 * means you overflowed your stack at some point, which can cause all
 * kinds of mysterious other things to happen.
 *
 * Note that when ->t_stack is NULL, which is the case if the stack
 * cannot be freed (which in turn is the case if the stack is the boot
 * stack, and the thread is the boot thread) this doesn't do anything.
 */
static
void
thread_checkstack(struct thread *thread)
{
	if (thread->t_stack != NULL) {
		KASSERT(((uint32_t*)thread->t_stack)[0] == THREAD_STACK_MAGIC);
		KASSERT(((uint32_t*)thread->t_stack)[1] == THREAD_STACK_MAGIC);
		KASSERT(((uint32_t*)thread->t_stack)[2] == THREAD_STACK_MAGIC);
		KASSERT(((uint32_t*)thread->t_stack)[3] == THREAD_STACK_MAGIC);
	}
}

/*
 * Create a thread. This is used both to create a first thread
 * for each CPU and to create subsequent forked threads.
 */
static
struct thread *
thread_create(const char *name)
{
	struct thread *thread;

	DEBUGASSERT(name != NULL);

	thread = kmalloc(sizeof(*thread));
	if (thread == NULL) {
		return NULL;
	}

	thread->t_name = kstrdup(name);
	if (thread->t_name == NULL) {
		kfree(thread);
		return NULL;
	}
	thread->t_wchan_name = "NEW";
	thread->t_state = S_READY;

	/* Thread subsystem fields */
	thread_machdep_init(&thread->t_machdep);
	threadlistnode_init(&thread->t_listnode, thread);
	thread->t_stack = NULL;
	thread->t_context = NULL;
	thread->t_cpu = NULL;
	thread->t_proc = NULL;

	/* Interrupt state fields */
	thread->t_in_interrupt = false;
	thread->t_curspl = IPL_HIGH;
	thread->t_iplhigh_count = 1; /* corresponding to t_curspl */

	/* If you add to struct thread, be sure to initialize here */

	return thread;
}

/*
 * Create a CPU structure. This is used for the bootup CPU and
 * also for secondary CPUs.
 *
 * The hardware number (the number assigned by firmware or system
 * board config or whatnot) is tracked separately because it is not
 * necessarily anything sane or meaningful.
 */
struct cpu *
cpu_create(unsigned hardware_number)
{
	struct cpu *c;
	int result;
	char namebuf[16];

	c = kmalloc(sizeof(*c));
	if (c == NULL) {
		panic("cpu_create: Out of memory\n");
	}

	c->c_self = c;
	c->c_hardware_number = hardware_number;

	c->c_curthread = NULL;
	threadlist_init(&c->c_zombies);
	c->c_hardclocks = 0;
	c->c_spinlocks = 0;

	c->c_isidle = false;
	threadlist_init(&c->c_runqueue);
	spinlock_init(&c->c_runqueue_lock);

	c->c_ipi_pending = 0;
	c->c_numshootdown = 0;
	spinlock_init(&c->c_ipi_lock);

	result = cpuarray_add(&allcpus, c, &c->c_number);
	if (result != 0) {
		panic("cpu_create: array_add: %s\n", strerror(result));
	}

	snprintf(namebuf, sizeof(namebuf), "<boot #%d>", c->c_number);
	c->c_curthread = thread_create(namebuf);
	if (c->c_curthread == NULL) {
		panic("cpu_create: thread_create failed\n");
	}
	c->c_curthread->t_cpu = c;

	if (c->c_number == 0) {
		/*
		 * Leave c->c_curthread->t_stack NULL for the boot
		 * cpu. This means we're using the boot stack, which
		 * can't be freed. (Exercise: what would it take to
		 * make it possible to free the boot stack?)
		 */
		/*c->c_curthread->t_stack = ... */
	}
	else {
		c->c_curthread->t_stack = kmalloc(STACK_SIZE);
		if (c->c_curthread->t_stack == NULL) {
			panic("cpu_create: couldn't allocate stack");
		}
		thread_checkstack_init(c->c_curthread);
	}

	/*
	 * If there is no curcpu (or curthread) yet, we are creating
	 * the first (boot) cpu. Initialize curcpu and curthread as
	 * early as possible so that other code can take locks without
	 * exploding.
	 */
	if (!CURCPU_EXISTS()) {
		/*
		 * Initializing curcpu and curthread is
		 * machine-dependent because either of curcpu and
		 * curthread might be defined in terms of the other.
		 */
		INIT_CURCPU(c, c->c_curthread);

		/*
		 * Now make sure both t_cpu and c_curthread are
		 * set. This might be partially redundant with
		 * INIT_CURCPU depending on how things are defined.
		 */
		curthread->t_cpu = curcpu;
		curcpu->c_curthread = curthread;
	}

	result = proc_addthread(kproc, c->c_curthread);
	if (result) {
		panic("cpu_create: proc_addthread:: %s\n", strerror(result));
	}

	cpu_machdep_init(c);

	return c;
}

/*
 * Destroy a thread.
 *
 * This function cannot be called in the victim thread's own context.
 * Nor can it be called on a running thread.
 *
 * (Freeing the stack you're actually using to run is ... inadvisable.)
 */
static
void
thread_destroy(struct thread *thread)
{
	KASSERT(thread != curthread);
	KASSERT(thread->t_state != S_RUN);

	/*
	 * If you add things to struct thread, be sure to clean them up
	 * either here or in thread_exit(). (And not both...)
	 */

	/* Thread subsystem fields */
	KASSERT(thread->t_proc == NULL);
	if (thread->t_stack != NULL) {
		kfree(thread->t_stack);
	}
	threadlistnode_cleanup(&thread->t_listnode);
	thread_machdep_cleanup(&thread->t_machdep);

	/* sheer paranoia */
	thread->t_wchan_name = "DESTROYED";

	kfree(thread->t_name);
	kfree(thread);
}

/*
 * Clean up zombies. (Zombies are threads that have exited but still
 * need to have thread_destroy called on them.)
 *
 * The list of zombies is per-cpu.
 */
static
void
exorcise(void)
{
	struct thread *z;

	while ((z = threadlist_remhead(&curcpu->c_zombies)) != NULL) {
		KASSERT(z != curthread);
		KASSERT(z->t_state == S_ZOMBIE);
		thread_destroy(z);
	}
}

/*
 * On panic, stop the thread system (as much as is reasonably
 * possible) to make sure we don't end up letting any other threads
 * run.
 */
void
thread_panic(void)
{
	/*
	 * Kill off other CPUs.
	 *
	 * We could wait for them to stop, except that they might not.
	 */
	ipi_broadcast(IPI_PANIC);

	/*
	 * Drop runnable threads on the floor.
	 *
	 * Don't try to get the run queue lock; we might not be able
	 * to.  Instead, blat the list structure by hand, and take the
	 * risk that it might not be quite atomic.
	 */
	curcpu->c_runqueue.tl_count = 0;
	curcpu->c_runqueue.tl_head.tln_next = &curcpu->c_runqueue.tl_tail;
	curcpu->c_runqueue.tl_tail.tln_prev = &curcpu->c_runqueue.tl_head;

	/*
	 * Ideally, we want to make sure sleeping threads don't wake
	 * up and start running. However, there's no good way to track
	 * down all the wchans floating around the system. Another
	 * alternative would be to set a global flag to make the wchan
	 * wakeup operations do nothing; but that would mean we
	 * ourselves couldn't sleep to wait for an I/O completion
	 * interrupt, and we'd like to be able to do that if the
	 * system isn't that badly hosed.
	 *
	 * So, do nothing else here.
	 *
	 * This may prove inadequate in practice and further steps
	 * might be needed. It may also be necessary to go through and
	 * forcibly unlock all locks or the like...
	 */
}

/*
 * At system shutdown, ask the other CPUs to switch off.
 */
void
thread_shutdown(void)
{
	/*
	 * Stop the other CPUs.
	 *
	 * We should probably wait for them to stop and shut them off
	 * on the system board.
	 */
	ipi_broadcast(IPI_OFFLINE);
}

/*
 * Thread system initialization.
 */
void
thread_bootstrap(void)
{
	cpuarray_init(&allcpus);

	/*
	 * Create the cpu structure for the bootup CPU, the one we're
	 * currently running on. Assume the hardware number is 0; that
	 * might be updated later by mainbus-type code. This also
	 * creates a thread structure for the first thread, the one
	 * that's already implicitly running when the kernel is
	 * started from the bootloader.
	 */
	KASSERT(CURCPU_EXISTS() == false);
	(void)cpu_create(0);
	KASSERT(CURCPU_EXISTS() == true);

	/* cpu_create() should also have set t_proc. */
	KASSERT(curcpu != NULL);
	KASSERT(curthread != NULL);
	KASSERT(curthread->t_proc != NULL);
	KASSERT(curthread->t_proc == kproc);

	/* Done */
}

/*
 * New CPUs come here once MD initialization is finished. curthread
 * and curcpu should already be initialized.
 *
 * Other than clearing thread_start_cpus() to continue, we don't need
 * to do anything. The startup thread can just exit; we only need it
 * to be able to get into thread_switch() properly.
 */
void
cpu_hatch(unsigned software_number)
{
	char buf[64];

	KASSERT(curcpu != NULL);
	KASSERT(curthread != NULL);
	KASSERT(curcpu->c_number == software_number);

	spl0();
	cpu_identify(buf, sizeof(buf));

	kprintf("cpu%u: %s\n", software_number, buf);

	V(cpu_startup_sem);
	thread_exit();
}

/*
 * Start up secondary cpus. Called from boot().
 */
void
thread_start_cpus(void)
{
	char buf[64];
	unsigned i;

	cpu_identify(buf, sizeof(buf));
	kprintf("cpu0: %s\n", buf);

	cpu_startup_sem = sem_create("cpu_hatch", 0);
	mainbus_start_cpus();

	for (i=0; i<cpuarray_num(&allcpus) - 1; i++) {
		P(cpu_startup_sem);
	}
	sem_destroy(cpu_startup_sem);
	cpu_startup_sem = NULL;
}

/*
 * Make a thread runnable.
 *
 * targetcpu might be curcpu; it might not be, too.
 */
static
void
thread_make_runnable(struct thread *target, bool already_have_lock)
{
	struct cpu *targetcpu;

	/* Lock the run queue of the target thread's cpu. */
	targetcpu = target->t_cpu;

	if (already_have_lock) {
		/* The target thread's cpu should be already locked. */
		KASSERT(spinlock_do_i_hold(&targetcpu->c_runqueue_lock));
	}
	else {
		spinlock_acquire(&targetcpu->c_runqueue_lock);
	}

	/* Target thread is now ready to run; put it on the run queue. */
	target->t_state = S_READY;
	threadlist_addtail(&targetcpu->c_runqueue, target);

	if (targetcpu->c_isidle && targetcpu != curcpu->c_self) {
		/*
		 * Other processor is idle; send interrupt to make
		 * sure it unidles.
		 */
		ipi_send(targetcpu, IPI_UNIDLE);
	}

	if (!already_have_lock) {
		spinlock_release(&targetcpu->c_runqueue_lock);
	}
}

/*
 * Create a new thread based on an existing one.
 *
 * The new thread has name NAME, and starts executing in function
 * ENTRYPOINT. DATA1 and DATA2 are passed to ENTRYPOINT.
 *
 * The new thread is created in the process P. If P is null, the
 * process is inherited from the caller. It will start on the same CPU
 * as the caller, unless the scheduler intervenes first.
 */
int
thread_fork(const char *name,
	    struct proc *proc,
	    void (*entrypoint)(void *data1, unsigned long data2),
	    void *data1, unsigned long data2)
{
	struct thread *newthread;
	int result;

	newthread = thread_create(name);
	if (newthread == NULL) {
		return ENOMEM;
	}

	/* Allocate a stack */
	newthread->t_stack = kmalloc(STACK_SIZE);
	if (newthread->t_stack == NULL) {
		thread_destroy(newthread);
		return ENOMEM;
	}
	thread_checkstack_init(newthread);

	/*
	 * Now we clone various fields from the parent thread.
	 */

	/* Thread subsystem fields */
	newthread->t_cpu = curthread->t_cpu;

	/* Attach the new thread to its process */
	if (proc == NULL) {
		proc = curthread->t_proc;
	}
	result = proc_addthread(proc, newthread);
	if (result) {
		/* thread_destroy will clean up the stack */
		thread_destroy(newthread);
		return result;
	}

	/*
	 * Because new threads come out holding the cpu runqueue lock
	 * (see notes at bottom of thread_switch), we need to account
	 * for the spllower() that will be done releasing it.
	 */
	newthread->t_iplhigh_count++;

	/* Set up the switchframe so entrypoint() gets called */
	switchframe_init(newthread, entrypoint, data1, data2);

	/* Lock the current cpu's run queue and make the new thread runnable */
	thread_make_runnable(newthread, false);

	return 0;
}

/*
 * High level, machine-independent context switch code.
 *
 * The current thread is queued appropriately and its state is changed
 * to NEWSTATE; another thread to run is selected and switched to.
 *
 * If NEWSTATE is S_SLEEP, the thread is queued on the wait channel
 * WC, protected by the spinlock LK. Otherwise WC and Lk should be
 * NULL.
 */
static
void
thread_switch(threadstate_t newstate, struct wchan *wc, struct spinlock *lk)
{
	struct thread *cur, *next;
	int spl;

	DEBUGASSERT(curcpu->c_curthread == curthread);
	DEBUGASSERT(curthread->t_cpu == curcpu->c_self);

	/* Explicitly disable interrupts on this processor */
	spl = splhigh();

	cur = curthread;

	/*
	 * If we're idle, return without doing anything. This happens
	 * when the timer interrupt interrupts the idle loop.
	 */
	if (curcpu->c_isidle) {
		splx(spl);
		return;
	}

	/* Check the stack guard band. */
	thread_checkstack(cur);

	/* Lock the run queue. */
	spinlock_acquire(&curcpu->c_runqueue_lock);

	/* Micro-optimization: if nothing to do, just return */
	if (newstate == S_READY && threadlist_isempty(&curcpu->c_runqueue)) {
		spinlock_release(&curcpu->c_runqueue_lock);
		splx(spl);
		return;
	}

	/* Put the thread in the right place. */
	switch (newstate) {
	    case S_RUN:
		panic("Illegal S_RUN in thread_switch\n");
	    case S_READY:
		thread_make_runnable(cur, true /*have lock*/);
		break;
	    case S_SLEEP:
		cur->t_wchan_name = wc->wc_name;
		/*
		 * Add the thread to the list in the wait channel, and
		 * unlock same. To avoid a race with someone else
		 * calling wchan_wake*, we must keep the wchan's
		 * associated spinlock locked from the point the
		 * caller of wchan_sleep locked it until the thread is
		 * on the list.
		 */
		threadlist_addtail(&wc->wc_threads, cur);
		spinlock_release(lk);
		break;
	    case S_ZOMBIE:
		cur->t_wchan_name = "ZOMBIE";
		threadlist_addtail(&curcpu->c_zombies, cur);
		break;
	}
	cur->t_state = newstate;

	/*
	 * Get the next thread. While there isn't one, call cpu_idle().
	 * curcpu->c_isidle must be true when cpu_idle is
	 * called. Unlock the runqueue while idling too, to make sure
	 * things can be added to it.
	 *
	 * Note that we don't need to unlock the runqueue atomically
	 * with idling; becoming unidle requires receiving an
	 * interrupt (either a hardware interrupt or an interprocessor
	 * interrupt from another cpu posting a wakeup) and idling
	 * *is* atomic with respect to re-enabling interrupts.
	 *
	 * Note that c_isidle becomes true briefly even if we don't go
	 * idle. However, because one is supposed to hold the runqueue
	 * lock to look at it, this should not be visible or matter.
	 */

	/* The current cpu is now idle. */
	curcpu->c_isidle = true;
	do {
		next = threadlist_remhead(&curcpu->c_runqueue);
		if (next == NULL) {
			spinlock_release(&curcpu->c_runqueue_lock);
			cpu_idle();
			spinlock_acquire(&curcpu->c_runqueue_lock);
		}
	} while (next == NULL);
	curcpu->c_isidle = false;

	/*
	 * Note that curcpu->c_curthread may be the same variable as
	 * curthread and it may not be, depending on how curthread and
	 * curcpu are defined by the MD code. We'll assign both and
	 * assume the compiler will optimize one away if they're the
	 * same.
	 */
	curcpu->c_curthread = next;
	curthread = next;

	/* do the switch (in assembler in switch.S) */
	switchframe_switch(&cur->t_context, &next->t_context);

	/*
	 * When we get to this point we are either running in the next
	 * thread, or have come back to the same thread again,
	 * depending on how you look at it. That is,
	 * switchframe_switch returns immediately in another thread
	 * context, which in general will be executing here with a
	 * different stack and different values in the local
	 * variables. (Although new threads go to thread_startup
	 * instead.) But, later on when the processor, or some
	 * processor, comes back to the previous thread, it's also
	 * executing here with the *same* value in the local
	 * variables.
	 *
	 * The upshot, however, is as follows:
	 *
	 *    - The thread now currently running is "cur", not "next",
	 *      because when we return from switchrame_switch on the
	 *      same stack, we're back to the thread that
	 *      switchframe_switch call switched away from, which is
	 *      "cur".
	 *
	 *    - "cur" is _not_ the thread that just *called*
	 *      switchframe_switch.
	 *
	 *    - If newstate is S_ZOMB we never get back here in that
	 *      context at all.
	 *
	 *    - If the thread just chosen to run ("next") was a new
	 *      thread, we don't get to this code again until
	 *      *another* context switch happens, because when new
	 *      threads return from switchframe_switch they teleport
	 *      to thread_startup.
	 *
	 *    - At this point the thread whose stack we're now on may
	 *      have been migrated to another cpu since it last ran.
	 *
	 * The above is inherently confusing and will probably take a
	 * while to get used to.
	 *
	 * However, the important part is that code placed here, after
	 * the call to switchframe_switch, does not necessarily run on
	 * every context switch. Thus any such code must be either
	 * skippable on some switches or also called from
	 * thread_startup.
	 */


	/* Clear the wait channel and set the thread state. */
	cur->t_wchan_name = NULL;
	cur->t_state = S_RUN;

	/* Unlock the run queue. */
	spinlock_release(&curcpu->c_runqueue_lock);

	/* Activate our address space in the MMU. */
	as_activate();

	/* Clean up dead threads. */
	exorcise();

	/* Turn interrupts back on. */
	splx(spl);
}

/*
 * This function is where new threads start running. The arguments
 * ENTRYPOINT, DATA1, and DATA2 are passed through from thread_fork.
 *
 * Because new code comes here from inside the middle of
 * thread_switch, the beginning part of this function must match the
 * tail of thread_switch.
 */
void
thread_startup(void (*entrypoint)(void *data1, unsigned long data2),
	       void *data1, unsigned long data2)
{
	struct thread *cur;

	cur = curthread;

	/* Clear the wait channel and set the thread state. */
	cur->t_wchan_name = NULL;
	cur->t_state = S_RUN;

	/* Release the runqueue lock acquired in thread_switch. */
	spinlock_release(&curcpu->c_runqueue_lock);

	/* Activate our address space in the MMU. */
	as_activate();

	/* Clean up dead threads. */
	exorcise();

	/* Enable interrupts. */
	spl0();

	/* Call the function. */
	entrypoint(data1, data2);

	/* Done. */
	thread_exit();
}

/*
 * Cause the current thread to exit.
 *
 * The parts of the thread structure we don't actually need to run
 * should be cleaned up right away. The rest has to wait until
 * thread_destroy is called from exorcise().
 *
 * Does not return.
 */
void
thread_exit(void)
{
	struct thread *cur;

	cur = curthread;

	/*
	 * Detach from our process. You might need to move this action
	 * around, depending on how your wait/exit works.
	 */
	proc_remthread(cur);

	/* Make sure we *are* detached (move this only if you're sure!) */
	KASSERT(cur->t_proc == NULL);

	/* Check the stack guard band. */
	thread_checkstack(cur);

	/* Interrupts off on this processor */
        splhigh();
	thread_switch(S_ZOMBIE, NULL, NULL);
	panic("braaaaaaaiiiiiiiiiiinssssss\n");
}

/*
 * Yield the cpu to another process, but stay runnable.
 */
void
thread_yield(void)
{
	thread_switch(S_READY, NULL, NULL);
}

////////////////////////////////////////////////////////////

/*
 * Scheduler.
 *
 * This is called periodically from hardclock(). It should reshuffle
 * the current CPU's run queue by job priority.
 */

void
schedule(void)
{
	/*
	 * You can write this. If we do nothing, threads will run in
	 * round-robin fashion.
	 */
}

/*
 * Thread migration.
 *
 * This is also called periodically from hardclock(). If the current
 * CPU is busy and other CPUs are idle, or less busy, it should move
 * threads across to those other other CPUs.
 *
 * Migrating threads isn't free because of cache affinity; a thread's
 * working cache set will end up having to be moved to the other CPU,
 * which is fairly slow. The tradeoff between this performance loss
 * and the performance loss due to underutilization of some CPUs is
 * something that needs to be tuned and probably is workload-specific.
 *
 * For here and now, because we know we're running on System/161 and
 * System/161 does not (yet) model such cache effects, we'll be very
 * aggressive.
 */
void
thread_consider_migration(void)
{
	unsigned my_count, total_count, one_share, to_send;
	unsigned i, numcpus;
	struct cpu *c;
	struct threadlist victims;
	struct thread *t;

	my_count = total_count = 0;
	numcpus = cpuarray_num(&allcpus);
	for (i=0; i<numcpus; i++) {
		c = cpuarray_get(&allcpus, i);
		spinlock_acquire(&c->c_runqueue_lock);
		total_count += c->c_runqueue.tl_count;
		if (c == curcpu->c_self) {
			my_count = c->c_runqueue.tl_count;
		}
		spinlock_release(&c->c_runqueue_lock);
	}

	one_share = DIVROUNDUP(total_count, numcpus);
	if (my_count < one_share) {
		return;
	}

	to_send = my_count - one_share;
	threadlist_init(&victims);
	spinlock_acquire(&curcpu->c_runqueue_lock);
	for (i=0; i<to_send; i++) {
		t = threadlist_remtail(&curcpu->c_runqueue);
		threadlist_addhead(&victims, t);
	}
	spinlock_release(&curcpu->c_runqueue_lock);

	for (i=0; i < numcpus && to_send > 0; i++) {
		c = cpuarray_get(&allcpus, i);
		if (c == curcpu->c_self) {
			continue;
		}
		spinlock_acquire(&c->c_runqueue_lock);
		while (c->c_runqueue.tl_count < one_share && to_send > 0) {
			t = threadlist_remhead(&victims);
			/*
			 * Ordinarily, curthread will not appear on
			 * the run queue. However, it can under the
			 * following circumstances:
			 *   - it went to sleep;
			 *   - the processor became idle, so it
			 *     remained curthread;
			 *   - it was reawakened, so it was put on the
			 *     run queue;
			 *   - and the processor hasn't fully unidled
			 *     yet, so all these things are still true.
			 *
			 * If the timer interrupt happens at (almost)
			 * exactly the proper moment, we can come here
			 * while things are in this state and see
			 * curthread. However, *migrating* curthread
			 * can cause bad things to happen (Exercise:
			 * Why? And what?) so shuffle it to the end of
			 * the list and decrement to_send in order to
			 * skip it. Then it goes back on our own run
			 * queue below.
			 */
			if (t == curthread) {
				threadlist_addtail(&victims, t);
				to_send--;
				continue;
			}

			t->t_cpu = c;
			threadlist_addtail(&c->c_runqueue, t);
			DEBUG(DB_THREADS,
			      "Migrated thread %s: cpu %u -> %u",
			      t->t_name, curcpu->c_number, c->c_number);
			to_send--;
			if (c->c_isidle) {
				/*
				 * Other processor is idle; send
				 * interrupt to make sure it unidles.
				 */
				ipi_send(c, IPI_UNIDLE);
			}
		}
		spinlock_release(&c->c_runqueue_lock);
	}

	/*
	 * Because the code above isn't atomic, the thread counts may have
	 * changed while we were working and we may end up with leftovers.
	 * Don't panic; just put them back on our own run queue.
	 */
	if (!threadlist_isempty(&victims)) {
		spinlock_acquire(&curcpu->c_runqueue_lock);
		while ((t = threadlist_remhead(&victims)) != NULL) {
			threadlist_addtail(&curcpu->c_runqueue, t);
		}
		spinlock_release(&curcpu->c_runqueue_lock);
	}

	KASSERT(threadlist_isempty(&victims));
	threadlist_cleanup(&victims);
}

////////////////////////////////////////////////////////////

/*
 * Wait channel functions
 */

/*
 * Create a wait channel. NAME is a symbolic string name for it.
 * This is what's displayed by ps -alx in Unix.
 *
 * NAME should generally be a string constant. If it isn't, alternate
 * arrangements should be made to free it after the wait channel is
 * destroyed.
 */
struct wchan *
wchan_create(const char *name)
{
	struct wchan *wc;

	wc = kmalloc(sizeof(*wc));
	if (wc == NULL) {
		return NULL;
	}
	threadlist_init(&wc->wc_threads);
	wc->wc_name = name;

	return wc;
}

/*
 * Destroy a wait channel. Must be empty and unlocked.
 * (The corresponding cleanup functions require this.)
 */
void
wchan_destroy(struct wchan *wc)
{
	threadlist_cleanup(&wc->wc_threads);
	kfree(wc);
}

/*
 * Yield the cpu to another process, and go to sleep, on the specified
 * wait channel WC, whose associated spinlock is LK. Calling wakeup on
 * the channel will make the thread runnable again. The spinlock must
 * be locked. The call to thread_switch unlocks it; we relock it
 * before returning.
 */
void
wchan_sleep(struct wchan *wc, struct spinlock *lk)
{
	/* may not sleep in an interrupt handler */
	KASSERT(!curthread->t_in_interrupt);

	/* must hold the spinlock */
	KASSERT(spinlock_do_i_hold(lk));

	/* must not hold other spinlocks */
	KASSERT(curcpu->c_spinlocks == 1);

	thread_switch(S_SLEEP, wc, lk);
	spinlock_acquire(lk);
}

/*
 * Wake up one thread sleeping on a wait channel.
 */
void
wchan_wakeone(struct wchan *wc, struct spinlock *lk)
{
	struct thread *target;

	KASSERT(spinlock_do_i_hold(lk));

	/* Grab a thread from the channel */
	target = threadlist_remhead(&wc->wc_threads);

	if (target == NULL) {
		/* Nobody was sleeping. */
		return;
	}

	/*
	 * Note that thread_make_runnable acquires a runqueue lock
	 * while we're holding LK. This is ok; all spinlocks
	 * associated with wchans must come before the runqueue locks,
	 * as we also bridge from the wchan lock to the runqueue lock
	 * in thread_switch.
	 */

	thread_make_runnable(target, false);
}

/*
 * Wake up all threads sleeping on a wait channel.
 */
void
wchan_wakeall(struct wchan *wc, struct spinlock *lk)
{
	struct thread *target;
	struct threadlist list;

	KASSERT(spinlock_do_i_hold(lk));

	threadlist_init(&list);

	/*
	 * Grab all the threads from the channel, moving them to a
	 * private list.
	 */
	while ((target = threadlist_remhead(&wc->wc_threads)) != NULL) {
		threadlist_addtail(&list, target);
	}

	/*
	 * We could conceivably sort by cpu first to cause fewer lock
	 * ops and fewer IPIs, but for now at least don't bother. Just
	 * make each thread runnable.
	 */
	while ((target = threadlist_remhead(&list)) != NULL) {
		thread_make_runnable(target, false);
	}

	threadlist_cleanup(&list);
}

/*
 * Return nonzero if there are no threads sleeping on the channel.
 * This is meant to be used only for diagnostic purposes.
 */
bool
wchan_isempty(struct wchan *wc, struct spinlock *lk)
{
	bool ret;

	KASSERT(spinlock_do_i_hold(lk));
	ret = threadlist_isempty(&wc->wc_threads);

	return ret;
}

////////////////////////////////////////////////////////////

/*
 * Machine-independent IPI handling
 */

/*
 * Send an IPI (inter-processor interrupt) to the specified CPU.
 */
void
ipi_send(struct cpu *target, int code)
{
	KASSERT(code >= 0 && code < 32);

	spinlock_acquire(&target->c_ipi_lock);
	target->c_ipi_pending |= (uint32_t)1 << code;
	mainbus_send_ipi(target);
	spinlock_release(&target->c_ipi_lock);
}

/*
 * Send an IPI to all CPUs.
 */
void
ipi_broadcast(int code)
{
	unsigned i;
	struct cpu *c;

	for (i=0; i < cpuarray_num(&allcpus); i++) {
		c = cpuarray_get(&allcpus, i);
		if (c != curcpu->c_self) {
			ipi_send(c, code);
		}
	}
}

/*
 * Send a TLB shootdown IPI to the specified CPU.
 */
void
ipi_tlbshootdown(struct cpu *target, const struct tlbshootdown *mapping)
{
	unsigned n;

	spinlock_acquire(&target->c_ipi_lock);

	n = target->c_numshootdown;
	if (n == TLBSHOOTDOWN_MAX) {
		/*
		 * If you have problems with this panic going off,
		 * consider: (1) increasing the maximum, (2) putting
		 * logic here to sleep until space appears (may
		 * interact awkwardly with VM system locking), (3)
		 * putting logic here to coalesce requests together,
		 * and/or (4) improving VM system state tracking to
		 * reduce the number of unnecessary shootdowns.
		 */
		panic("ipi_tlbshootdown: Too many shootdowns queued\n");
	}
	else {
		target->c_shootdown[n] = *mapping;
		target->c_numshootdown = n+1;
	}

	target->c_ipi_pending |= (uint32_t)1 << IPI_TLBSHOOTDOWN;
	mainbus_send_ipi(target);

	spinlock_release(&target->c_ipi_lock);
}

/*
 * Handle an incoming interprocessor interrupt.
 */
void
interprocessor_interrupt(void)
{
	uint32_t bits;
	unsigned i;

	spinlock_acquire(&curcpu->c_ipi_lock);
	bits = curcpu->c_ipi_pending;

	if (bits & (1U << IPI_PANIC)) {
		/* panic on another cpu - just stop dead */
		spinlock_release(&curcpu->c_ipi_lock);
		cpu_halt();
	}
	if (bits & (1U << IPI_OFFLINE)) {
		/* offline request */
		spinlock_release(&curcpu->c_ipi_lock);
		spinlock_acquire(&curcpu->c_runqueue_lock);
		if (!curcpu->c_isidle) {
			kprintf("cpu%d: offline: warning: not idle\n",
				curcpu->c_number);
		}
		spinlock_release(&curcpu->c_runqueue_lock);
		kprintf("cpu%d: offline.\n", curcpu->c_number);
		cpu_halt();
	}
	if (bits & (1U << IPI_UNIDLE)) {
		/*
		 * The cpu has already unidled itself to take the
		 * interrupt; don't need to do anything else.
		 */
	}
	if (bits & (1U << IPI_TLBSHOOTDOWN)) {
		/*
		 * Note: depending on your VM system locking you might
		 * need to release the ipi lock while calling
		 * vm_tlbshootdown.
		 */
		for (i=0; i<curcpu->c_numshootdown; i++) {
			vm_tlbshootdown(&curcpu->c_shootdown[i]);
		}
		curcpu->c_numshootdown = 0;
	}

	curcpu->c_ipi_pending = 0;
	spinlock_release(&curcpu->c_ipi_lock);
}
