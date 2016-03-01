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
 * crash.c
 *
 * 	Commit a variety of exceptions, primarily address faults.
 *
 * Once the basic system calls assignment is complete, none of these
 * should crash the kernel.
 *
 * They should all, however, terminate this program, except for the
 * one that writes to the code segment. (That one won't cause program
 * termination until/unless you implement read-only segments in your
 * VM system.)
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <err.h>
#include <test161/test161.h>


#if defined(__mips__)
#define KERNEL_ADDR	0x80000000
#define INVAL_ADDR	0x40000000
#define INSN_TYPE	uint32_t
#define INVAL_INSN	0x0000003f
#else
#error "Please fix this"
#endif

#define MAGIC		123456

typedef void (*func)(void);

static int forking = 1;

static
void
read_from_null(void)
{
	int *null = NULL;
	volatile int x;

	x = *null;

	// gcc 4.8 improperly demands this
	(void)x;
}

static
void
read_from_inval(void)
{
	int *ptr = (int *) INVAL_ADDR;
	volatile int x;

	x = *ptr;

	// gcc 4.8 improperly demands this
	(void)x;
}

static
void
read_from_kernel(void)
{
	int *ptr = (int *) KERNEL_ADDR;
	volatile int x;

	x = *ptr;

	// gcc 4.8 improperly demands this
	(void)x;
}

static
void
write_to_null(void)
{
	int *null = NULL;
	*null = 6;
}

static
void
write_to_inval(void)
{
	int *ptr = (int *) INVAL_ADDR;
	*ptr = 8;
}

static
void
write_to_code(void)
{
	INSN_TYPE *x = (INSN_TYPE *)write_to_code;
	*x = INVAL_INSN;
}

static
void
write_to_kernel(void)
{
	int *ptr = (int *) KERNEL_ADDR;
	*ptr = 8;
}

static
void
jump_to_null(void)
{
	func f = NULL;
	f();
}

static
void
jump_to_inval(void)
{
	func f = (func) INVAL_ADDR;
	f();
}

static
void
jump_to_kernel(void)
{
	func f = (func) KERNEL_ADDR;
	f();
}


static
void
illegal_instruction(void)
{
#if defined(__mips__)
	asm(".long 0x0000003f");
#else
#error "Please fix this"
#endif
}

static
void
alignment_error(void)
{
	int x;
	int *ptr, *badptr;
	volatile uintptr_t ptrval;
	volatile int j;

	x = 0;
	ptr = &x;
	/*
	 * Try to hide what's going on from gcc; gcc 4.8 seems to
	 * detect the unaligned access and issue unaligned read
	 * instructions for it, so then it doesn't fault. Feh.
	 */
	ptrval = (uintptr_t)ptr;
	ptrval++;
	badptr = (int *)ptrval;

	j = *badptr;

	// gcc 4.8 improperly demands this
	(void)j;
}

static
void
divide_by_zero(void)
{
	volatile int x = 6;
	volatile int z = 0;
	volatile int a;

	a = x/z;

	// gcc 4.8 improperly demands this
	(void)a;
}

static
void
mod_by_zero(void)
{
	volatile int x = 6;
	volatile int z = 0;
	volatile int a;

	a = x%z;

	// gcc 4.8 improperly demands this
	(void)a;
}

static
void
recurse_inf(void)
{
	volatile char buf[16];

	buf[0] = 0;
	recurse_inf();
	buf[0] = 1;

	// gcc 4.8 improperly demands this
	(void)buf;
}


static
struct {
	int ch;
	const char *name;
	func f;
	int sig;
} ops[] = {
	{ 'a', "read from NULL",            read_from_null,      SIGSEGV },
	{ 'b', "read from invalid address", read_from_inval,     SIGSEGV },
	{ 'c', "read from kernel address",  read_from_kernel,    SIGBUS },
	{ 'd', "write to NULL",             write_to_null,       SIGSEGV },
	{ 'e', "write to invalid address",  write_to_inval,      SIGSEGV },
	{ 'f', "write to code segment",     write_to_code,       SIGSEGV },
	{ 'g', "write to kernel address",   write_to_kernel,     SIGBUS },
	{ 'h', "jump to NULL",              jump_to_null,        SIGSEGV },
	{ 'i', "jump to invalid address",   jump_to_inval,       SIGSEGV },
	{ 'j', "jump to kernel address",    jump_to_kernel,      SIGBUS },
	{ 'k', "alignment error",           alignment_error,     SIGBUS },
	{ 'l', "illegal instruction",       illegal_instruction, SIGILL },
	{ 'm', "divide by zero",            divide_by_zero,      SIGTRAP },
	{ 'n', "mod by zero",               mod_by_zero,         SIGTRAP },
	{ 'o', "Recurse infinitely",        recurse_inf,         SIGSEGV },
	{ 0, NULL, NULL, 0 }
};

static
void
runop(int op)
{
	int opindex;
	pid_t pid;
	int status;
	int ok;

	if (op=='*') {
		for (unsigned i=0; ops[i].name; i++) {
			runop(ops[i].ch);
		}
		return;
	}
	else if (op == '-') {
		forking = 0;
		warnx("Forking disabled - next try will be the last");
		return;
	}
	else if (op == '+') {
		forking = 1;
		warnx("Forking enabled.");
		return;
	}

	/* intentionally don't check if op is in bounds :) */
	opindex = op-'a';

	tprintf("Running: [%c] %s\n", ops[opindex].ch, ops[opindex].name);

	if (forking) {
		pid = fork();
		if (pid < 0) {
			/* error */
			err(1, "fork");
		}
		else if (pid > 0) {
			/* parent */
			if (waitpid(pid, &status, 0) < 0) {
				err(1, "waitpid");
			}
			ok = 0;
			if (WIFSIGNALED(status)) {
				tprintf("Signal %d\n", WTERMSIG(status));
				if (WTERMSIG(status) == ops[opindex].sig) {
					ok = 1;
				}
			}
			else {
				tprintf("Exit %d\n", WEXITSTATUS(status));
				if (WEXITSTATUS(status) == MAGIC) {
					ok = 1;
				}
			}
			if (ok) {
				tprintf("Ok.\n");
			}
			else {
				tprintf("FAILED: expected signal %d\n",
				       ops[opindex].sig);
			}
			tprintf("\n");
			return;
		}
	}
	/* child, or not forking */

	ops[opindex].f();

	if (op == 'f') {
		warnx(".... I guess you don't support read-only segments");
		/* use this magic signaling value so parent doesn't say FAIL */
		_exit(MAGIC);
	}
	errx(1, "I wasn't killed!");
}

static
void
ask(void)
{
	unsigned i;
	int op;

	while (1) {

		for (i=0; ops[i].name; i++) {
			tprintf("[%c] %s\n", ops[i].ch, ops[i].name);
		}
		tprintf("[-] Disable forking\n");
		tprintf("[+] Enable forking (default)\n");
		tprintf("[*] Run everything\n");
		tprintf("[!] Quit\n");

		tprintf("Choose: ");
		op = getchar();

		if (op == '!') {
			break;
		}

		runop(op);
	}
}

int
main(int argc, char **argv)
{
	if (argc == 0 || argc == 1) {
		/* no arguments */
		ask();
	}
	else {
		/* run the selected ops */
		for (int i=1; i<argc; i++) {
			for (size_t j=0; argv[i][j]; j++) {
				runop(argv[i][j]);
			}
		}
	}
	printf("Should print success\n");
	success(TEST161_SUCCESS, SECRET, "/testbin/crash");
	return 0;
}
