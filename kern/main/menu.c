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
#include <kern/errno.h>
#include <kern/reboot.h>
#include <kern/unistd.h>
#include <limits.h>
#include <lib.h>
#include <uio.h>
#include <clock.h>
#include <mainbus.h>
#include <synch.h>
#include <thread.h>
#include <proc.h>
#include <vfs.h>
#include <sfs.h>
#include <syscall.h>
#include <test.h>
#include "opt-sfs.h"
#include "opt-net.h"

/*
 * In-kernel menu and command dispatcher.
 */

#define _PATH_SHELL "/bin/sh"

#define MAXMENUARGS  16

////////////////////////////////////////////////////////////
//
// Command menu functions

/*
 * Function for a thread that runs an arbitrary userlevel program by
 * name.
 *
 * Note: this cannot pass arguments to the program. You may wish to
 * change it so it can, because that will make testing much easier
 * in the future.
 *
 * It copies the program name because runprogram destroys the copy
 * it gets by passing it to vfs_open().
 */
static
void
cmd_progthread(void *ptr, unsigned long nargs)
{
	char **args = ptr;
	char progname[128];
	int result;

	KASSERT(nargs >= 1);

	if (nargs > 2) {
		kprintf("Warning: argument passing from menu not supported\n");
	}

	/* Hope we fit. */
	KASSERT(strlen(args[0]) < sizeof(progname));

	strcpy(progname, args[0]);

	result = runprogram(progname);
	if (result) {
		kprintf("Running program %s failed: %s\n", args[0],
			strerror(result));
		return;
	}

	/* NOTREACHED: runprogram only returns on error. */
}

/*
 * Common code for cmd_prog and cmd_shell.
 *
 * Note that this does not wait for the subprogram to finish, but
 * returns immediately to the menu. This is usually not what you want,
 * so you should have it call your system-calls-assignment waitpid
 * code after forking.
 *
 * Also note that because the subprogram's thread uses the "args"
 * array and strings, until you do this a race condition exists
 * between that code and the menu input code.
 */
static
int
common_prog(int nargs, char **args)
{
	struct proc *proc;
	int result;

	/* Create a process for the new program to run in. */
	proc = proc_create_runprogram(args[0] /* name */);
	if (proc == NULL) {
		return ENOMEM;
	}

	result = thread_fork(args[0] /* thread name */,
			proc /* new process */,
			cmd_progthread /* thread function */,
			args /* thread arg */, nargs /* thread arg */);
	if (result) {
		kprintf("thread_fork failed: %s\n", strerror(result));
		proc_destroy(proc);
		return result;
	}

	/*
	 * The new process will be destroyed when the program exits...
	 * once you write the code for handling that.
	 */

	return 0;
}

/*
 * Command for running an arbitrary userlevel program.
 */
static
int
cmd_prog(int nargs, char **args)
{
	if (nargs < 2) {
		kprintf("Usage: p program [arguments]\n");
		return EINVAL;
	}

	/* drop the leading "p" */
	args++;
	nargs--;

	return common_prog(nargs, args);
}

/*
 * Command for starting the system shell.
 */
static
int
cmd_shell(int nargs, char **args)
{
	(void)args;
	if (nargs != 1) {
		kprintf("Usage: s\n");
		return EINVAL;
	}

	args[0] = (char *)_PATH_SHELL;

	return common_prog(nargs, args);
}

/*
 * Command for changing directory.
 */
static
int
cmd_chdir(int nargs, char **args)
{
	if (nargs != 2) {
		kprintf("Usage: cd directory\n");
		return EINVAL;
	}

	return vfs_chdir(args[1]);
}

/*
 * Command for printing the current directory.
 */
static
int
cmd_pwd(int nargs, char **args)
{
	char buf[PATH_MAX+1];
	int result;
	struct iovec iov;
	struct uio ku;

	(void)nargs;
	(void)args;

	uio_kinit(&iov, &ku, buf, sizeof(buf)-1, 0, UIO_READ);
	result = vfs_getcwd(&ku);
	if (result) {
		kprintf("vfs_getcwd failed (%s)\n", strerror(result));
		return result;
	}

	/* null terminate */
	buf[sizeof(buf)-1-ku.uio_resid] = 0;

	/* print it */
	kprintf("%s\n", buf);

	return 0;
}

/*
 * Command for running sync.
 */
static
int
cmd_sync(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	vfs_sync();

	return 0;
}

/*
 * Command for dropping to the debugger.
 */
static
int
cmd_debug(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	mainbus_debugger();

	return 0;
}

/*
 * Command for doing an intentional panic.
 */
static
int
cmd_panic(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	panic("User requested panic\n");
	return 0;
}

/*
 * Subthread for intentially deadlocking.
 */
struct deadlock {
	struct lock *lock1;
	struct lock *lock2;
};

static
void
cmd_deadlockthread(void *ptr, unsigned long num)
{
	struct deadlock *dl = ptr;

	(void)num;

	/* If it doesn't wedge right away, keep trying... */
	while (1) {
		lock_acquire(dl->lock2);
		lock_acquire(dl->lock1);
		kprintf("+");
		lock_release(dl->lock1);
		lock_release(dl->lock2);
	}
}

/*
 * Command for doing an intentional deadlock.
 */
static
int
cmd_deadlock(int nargs, char **args)
{
	struct deadlock dl;
	int result;

	(void)nargs;
	(void)args;

	dl.lock1 = lock_create("deadlock1");
	if (dl.lock1 == NULL) {
		kprintf("lock_create failed\n");
		return ENOMEM;
	}
	dl.lock2 = lock_create("deadlock2");
	if (dl.lock2 == NULL) {
		lock_destroy(dl.lock1);
		kprintf("lock_create failed\n");
		return ENOMEM;
	}

	result = thread_fork(args[0] /* thread name */,
			NULL /* kernel thread */,
			cmd_deadlockthread /* thread function */,
			&dl /* thread arg */, 0 /* thread arg */);
	if (result) {
		kprintf("thread_fork failed: %s\n", strerror(result));
		lock_release(dl.lock1);
		lock_destroy(dl.lock2);
		lock_destroy(dl.lock1);
		return result;
	}

	/* If it doesn't wedge right away, keep trying... */
	while (1) {
		lock_acquire(dl.lock1);
		lock_acquire(dl.lock2);
		kprintf(".");
		lock_release(dl.lock2);
		lock_release(dl.lock1);
	}
	/* NOTREACHED */
	return 0;
}

/*
 * Command for shutting down.
 */
static
int
cmd_quit(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	vfs_sync();
	sys_reboot(RB_POWEROFF);
	thread_exit();
	return 0;
}

/*
 * Command for mounting a filesystem.
 */

/* Table of mountable filesystem types. */
static const struct {
	const char *name;
	int (*func)(const char *device);
} mounttable[] = {
#if OPT_SFS
	{ "sfs", sfs_mount },
#endif
};

static
int
cmd_mount(int nargs, char **args)
{
	char *fstype;
	char *device;
	unsigned i;

	if (nargs != 3) {
		kprintf("Usage: mount fstype device:\n");
		return EINVAL;
	}

	fstype = args[1];
	device = args[2];

	/* Allow (but do not require) colon after device name */
	if (device[strlen(device)-1]==':') {
		device[strlen(device)-1] = 0;
	}

	for (i=0; i<ARRAYCOUNT(mounttable); i++) {
		if (!strcmp(mounttable[i].name, fstype)) {
			return mounttable[i].func(device);
		}
	}
	kprintf("Unknown filesystem type %s\n", fstype);
	return EINVAL;
}

static
int
cmd_unmount(int nargs, char **args)
{
	char *device;

	if (nargs != 2) {
		kprintf("Usage: unmount device:\n");
		return EINVAL;
	}

	device = args[1];

	/* Allow (but do not require) colon after device name */
	if (device[strlen(device)-1]==':') {
		device[strlen(device)-1] = 0;
	}

	return vfs_unmount(device);
}

/*
 * Command to set the "boot fs".
 *
 * The boot filesystem is the one that pathnames like /bin/sh with
 * leading slashes refer to.
 *
 * The default bootfs is "emu0".
 */
static
int
cmd_bootfs(int nargs, char **args)
{
	char *device;

	if (nargs != 2) {
		kprintf("Usage: bootfs device\n");
		return EINVAL;
	}

	device = args[1];

	/* Allow (but do not require) colon after device name */
	if (device[strlen(device)-1]==':') {
		device[strlen(device)-1] = 0;
	}

	return vfs_setbootfs(device);
}

static
int
cmd_kheapstats(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	kheap_printstats();

	return 0;
}

static
int
cmd_kheapgeneration(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	kheap_nextgeneration();

	return 0;
}

static
int
cmd_kheapdump(int nargs, char **args)
{
	if (nargs == 1) {
		kheap_dump();
	}
	else if (nargs == 2 && !strcmp(args[1], "all")) {
		kheap_dumpall();
	}
	else {
		kprintf("Usage: khdump [all]\n");
	}

	return 0;
}

////////////////////////////////////////
//
// Menus.

static
void
showmenu(const char *name, const char *x[])
{
	int ct, half, i;

	kprintf("\n");
	kprintf("%s\n", name);

	for (i=ct=0; x[i]; i++) {
		ct++;
	}
	half = (ct+1)/2;

	for (i=0; i<half; i++) {
		kprintf("    %-36s", x[i]);
		if (i+half < ct) {
			kprintf("%s", x[i+half]);
		}
		kprintf("\n");
	}

	kprintf("\n");
}

static const char *opsmenu[] = {
	"[s]       Shell                     ",
	"[p]       Other program             ",
	"[mount]   Mount a filesystem        ",
	"[unmount] Unmount a filesystem      ",
	"[bootfs]  Set \"boot\" filesystem     ",
	"[pf]      Print a file              ",
	"[cd]      Change directory          ",
	"[pwd]     Print current directory   ",
	"[sync]    Sync filesystems          ",
	"[debug]   Drop to debugger          ",
	"[panic]   Intentional panic         ",
	"[deadlock] Intentional deadlock     ",
	"[q]       Quit and shut down        ",
	NULL
};

static
int
cmd_opsmenu(int n, char **a)
{
	(void)n;
	(void)a;

	showmenu("OS/161 operations menu", opsmenu);
	return 0;
}

static const char *testmenu[] = {
	"[at]  Array test                    ",
	"[at2] Large array test              ",
	"[bt]  Bitmap test                   ",
	"[tlt] Threadlist test               ",
	"[km1] Kernel malloc test            ",
	"[km2] kmalloc stress test           ",
	"[km3] Large kmalloc test            ",
	"[km4] Multipage kmalloc test        ",
	"[tt1] Thread test 1                 ",
	"[tt2] Thread test 2                 ",
	"[tt3] Thread test 3                 ",
#if OPT_NET
	"[net] Network test                  ",
#endif
	"[sy1] Semaphore test                ",
	"[sy2] Lock test             (1)     ",
	"[sy3] CV test               (1)     ",
	"[sy4] CV test #2            (1)     ",
	"[semu1-22] Semaphore unit tests     ",
	"[fs1] Filesystem test               ",
	"[fs2] FS read stress                ",
	"[fs3] FS write stress               ",
	"[fs4] FS write stress 2             ",
	"[fs5] FS long stress                ",
	"[fs6] FS create stress              ",
	NULL
};

static
int
cmd_testmenu(int n, char **a)
{
	(void)n;
	(void)a;

	showmenu("OS/161 tests menu", testmenu);
	kprintf("    (1) These tests will fail until you finish the "
		"synch assignment.\n");
	kprintf("\n");

	return 0;
}

static const char *mainmenu[] = {
	"[?o] Operations menu                ",
	"[?t] Tests menu                     ",
	"[kh] Kernel heap stats              ",
	"[khgen] Next kernel heap generation ",
	"[khdump] Dump kernel heap           ",
	"[q] Quit and shut down              ",
	NULL
};

static
int
cmd_mainmenu(int n, char **a)
{
	(void)n;
	(void)a;

	showmenu("OS/161 kernel menu", mainmenu);
	return 0;
}

////////////////////////////////////////
//
// Command table.

static struct {
	const char *name;
	int (*func)(int nargs, char **args);
} cmdtable[] = {
	/* menus */
	{ "?",		cmd_mainmenu },
	{ "h",		cmd_mainmenu },
	{ "help",	cmd_mainmenu },
	{ "?o",		cmd_opsmenu },
	{ "?t",		cmd_testmenu },

	/* operations */
	{ "s",		cmd_shell },
	{ "p",		cmd_prog },
	{ "mount",	cmd_mount },
	{ "unmount",	cmd_unmount },
	{ "bootfs",	cmd_bootfs },
	{ "pf",		printfile },
	{ "cd",		cmd_chdir },
	{ "pwd",	cmd_pwd },
	{ "sync",	cmd_sync },
	{ "debug",	cmd_debug },
	{ "panic",	cmd_panic },
	{ "deadlock",	cmd_deadlock },
	{ "q",		cmd_quit },
	{ "exit",	cmd_quit },
	{ "halt",	cmd_quit },

	/* stats */
	{ "kh",         cmd_kheapstats },
	{ "khgen",      cmd_kheapgeneration },
	{ "khdump",     cmd_kheapdump },

	/* base system tests */
	{ "at",		arraytest },
	{ "at2",	arraytest2 },
	{ "bt",		bitmaptest },
	{ "tlt",	threadlisttest },
	{ "km1",	kmalloctest },
	{ "km2",	kmallocstress },
	{ "km3",	kmalloctest3 },
	{ "km4",	kmalloctest4 },
#if OPT_NET
	{ "net",	nettest },
#endif
	{ "tt1",	threadtest },
	{ "tt2",	threadtest2 },
	{ "tt3",	threadtest3 },
	{ "sy1",	semtest },

	/* synchronization assignment tests */
	{ "sy2",	locktest },
	{ "sy3",	cvtest },
	{ "sy4",	cvtest2 },

	/* semaphore unit tests */
	{ "semu1",	semu1 },
	{ "semu2",	semu2 },
	{ "semu3",	semu3 },
	{ "semu4",	semu4 },
	{ "semu5",	semu5 },
	{ "semu6",	semu6 },
	{ "semu7",	semu7 },
	{ "semu8",	semu8 },
	{ "semu9",	semu9 },
	{ "semu10",	semu10 },
	{ "semu11",	semu11 },
	{ "semu12",	semu12 },
	{ "semu13",	semu13 },
	{ "semu14",	semu14 },
	{ "semu15",	semu15 },
	{ "semu16",	semu16 },
	{ "semu17",	semu17 },
	{ "semu18",	semu18 },
	{ "semu19",	semu19 },
	{ "semu20",	semu20 },
	{ "semu21",	semu21 },
	{ "semu22",	semu22 },

	/* file system assignment tests */
	{ "fs1",	fstest },
	{ "fs2",	readstress },
	{ "fs3",	writestress },
	{ "fs4",	writestress2 },
	{ "fs5",	longstress },
	{ "fs6",	createstress },

	{ NULL, NULL }
};

/*
 * Process a single command.
 */
static
int
cmd_dispatch(char *cmd)
{
	struct timespec before, after, duration;
	char *args[MAXMENUARGS];
	int nargs=0;
	char *word;
	char *context;
	int i, result;

	for (word = strtok_r(cmd, " \t", &context);
	     word != NULL;
	     word = strtok_r(NULL, " \t", &context)) {

		if (nargs >= MAXMENUARGS) {
			kprintf("Command line has too many words\n");
			return E2BIG;
		}
		args[nargs++] = word;
	}

	if (nargs==0) {
		return 0;
	}

	for (i=0; cmdtable[i].name; i++) {
		if (*cmdtable[i].name && !strcmp(args[0], cmdtable[i].name)) {
			KASSERT(cmdtable[i].func!=NULL);

			gettime(&before);

			result = cmdtable[i].func(nargs, args);

			gettime(&after);
			timespec_sub(&after, &before, &duration);

			kprintf("Operation took %llu.%09lu seconds\n",
				(unsigned long long) duration.tv_sec,
				(unsigned long) duration.tv_nsec);

			return result;
		}
	}

	kprintf("%s: Command not found\n", args[0]);
	return EINVAL;
}

/*
 * Evaluate a command line that may contain multiple semicolon-delimited
 * commands.
 *
 * If "isargs" is set, we're doing command-line processing; print the
 * comamnds as we execute them and panic if the command is invalid or fails.
 */
static
void
menu_execute(char *line, int isargs)
{
	char *command;
	char *context;
	int result;

	for (command = strtok_r(line, ";", &context);
	     command != NULL;
	     command = strtok_r(NULL, ";", &context)) {

		if (isargs) {
			kprintf("OS/161 kernel: %s\n", command);
		}

		result = cmd_dispatch(command);
		if (result) {
			kprintf("Menu command failed: %s\n", strerror(result));
			if (isargs) {
				panic("Failure processing kernel arguments\n");
			}
		}
	}
}

/*
 * Command menu main loop.
 *
 * First, handle arguments passed on the kernel's command line from
 * the bootloader. Then loop prompting for commands.
 *
 * The line passed in from the bootloader is treated as if it had been
 * typed at the prompt. Semicolons separate commands; spaces and tabs
 * separate words (command names and arguments).
 *
 * So, for instance, to mount an SFS on lhd0 and make it the boot
 * filesystem, and then boot directly into the shell, one would use
 * the kernel command line
 *
 *      "mount sfs lhd0; bootfs lhd0; s"
 */

void
menu(char *args)
{
	char buf[64];

	menu_execute(args, 1);

	while (1) {
		kprintf("OS/161 kernel [? for menu]: ");
		kgets(buf, sizeof(buf));
		menu_execute(buf, 0);
	}
}
