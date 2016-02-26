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
 * sh - shell
 *
 * Usage:
 *     sh
 *     sh -c command
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <err.h>

#ifdef HOST
#include "hostcompat.h"
#endif

#ifndef NARG_MAX
/* no NARG_MAX on most unixes */
#define NARG_MAX 1024
#endif

/* avoid making this unreasonably large; causes problems under dumbvm */
#define CMDLINE_MAX 4096

/* struct to (portably) hold exit info */
struct exitinfo {
	unsigned val:8,
		signaled:1,
		stopped:1,
		coredump:1;
};

/* set to nonzero if __time syscall seems to work */
static int timing = 0;

/* array of backgrounded jobs (allows "foregrounding") */
#define MAXBG 128
static pid_t bgpids[MAXBG];

/*
 * can_bg
 * just checks for an open slot.
 */
static
int
can_bg(void)
{
	int i;

	for (i = 0; i < MAXBG; i++) {
		if (bgpids[i] == 0) {
			return 1;
		}
	}

	return 0;
}

/*
 * remember_bg
 * sticks the pid in an open slot in the background array.  note the assert --
 * better check can_bg before calling this.
 */
static
void
remember_bg(pid_t pid)
{
	int i;
	for (i = 0; i < MAXBG; i++) {
		if (bgpids[i] == 0) {
			bgpids[i] = pid;
			return;
		}
	}
	assert(0);
}

/*
 * constructor for exitinfo
 */
static
void
exitinfo_exit(struct exitinfo *ei, int code)
{
	ei->val = code;
	ei->signaled = 0;
	ei->stopped = 0;
	ei->coredump = 0;
}

/*
 * readstatus
 * unpack results from wait
 */
static
void
readstatus(int status, struct exitinfo *ei)
{
	if (WIFEXITED(status)) {
		ei->val = WEXITSTATUS(status);
		ei->signaled = 0;
		ei->stopped = 0;
		ei->coredump = 0;
	}
	else if (WIFSIGNALED(status) && WCOREDUMP(status)) {
		ei->val = WTERMSIG(status);
		ei->signaled = 1;
		ei->stopped = 0;
		ei->coredump = 1;
	}
	else if (WIFSIGNALED(status)) {
		ei->val = WTERMSIG(status);
		ei->signaled = 1;
		ei->stopped = 0;
		ei->coredump = 0;
	}
	else if (WIFSTOPPED(status)) {
		ei->val = WSTOPSIG(status);
		ei->signaled = 0;
		ei->stopped = 1;
		ei->coredump = 0;
	}
	else {
		printf("Invalid status code %d", status);
		ei->val = status;
		ei->signaled = 0;
		ei->stopped = 0;
		ei->coredump = 0;
	}

}

/*
 * printstatus
 * print results from wait
 */
static
void
printstatus(const struct exitinfo *ei, int printexitzero)
{
	if (ei->signaled && ei->coredump) {
		printf("Signal %d (core dumped)\n", ei->val);
	}
	else if (ei->signaled) {
		printf("Signal %d\n", ei->val);
	}
	else if (ei->stopped) {
		printf("Stopped on signal %d\n", ei->val);
	}
	else if (printexitzero || ei->val != 0) {
		printf("Exit %d\n", ei->val);
	}
}

/*
 * dowait
 * just does a waitpid.
 */
static
void
dowait(pid_t pid)
{
	struct exitinfo ei;
	int status;

	if (waitpid(pid, &status, 0) < 0) {
		warn("pid %d", pid);
	}
	else {
		printf("pid %d: ", pid);
		readstatus(status, &ei);
		printstatus(&ei, 1);
	}
}

#ifdef WNOHANG
/*
 * dowaitpoll
 * like dowait, but uses WNOHANG. returns true if we got something.
 */
static
int
dowaitpoll(pid_t pid)
{
	struct exitinfo ei;
	pid_t foundpid;
	int status;

	foundpid = waitpid(pid, &status, WNOHANG);
	if (foundpid < 0) {
		warn("pid %d", pid);
	}
	else if (foundpid != 0) {
		printf("pid %d: ", pid);
		readstatus(status, &ei);
		printstatus(&ei, 1);
		return 1;
	}
	return 0;
}

/*
 * waitpoll
 * poll all background jobs for having exited.
 */
static
void
waitpoll(void)
{
	int i;
	for (i=0; i < MAXBG; i++) {
		if (bgpids[i] != 0) {
			if (dowaitpoll(bgpids[i])) {
				bgpids[i] = 0;
			}
		}
	}
}
#endif /* WNOHANG */

/*
 * wait
 * allows the user to "foreground" a process by waiting on it.  without ps to
 * know the pids, this is a little tough to use with an arg, but without an
 * arg it will wait for all the background jobs.
 */
static
void
cmd_wait(int ac, char *av[], struct exitinfo *ei)
{
	int i;
	pid_t pid;

	if (ac == 2) {
		pid = atoi(av[1]);
		dowait(pid);
		for (i = 0; i < MAXBG; i++) {
			if (bgpids[i]==pid) {
				bgpids[i] = 0;
			}
		}
		exitinfo_exit(ei, 0);
		return;
	}
	else if (ac == 1) {
		for (i=0; i < MAXBG; i++) {
			if (bgpids[i] != 0) {
				dowait(bgpids[i]);
				bgpids[i] = 0;
			}
		}
		exitinfo_exit(ei, 0);
		return;
	}
	printf("Usage: wait [pid]\n");
	exitinfo_exit(ei, 1);
}

/*
 * chdir
 * just an interface to the system call.  no concept of home directory, so
 * require the directory.
 */
static
void
cmd_chdir(int ac, char *av[], struct exitinfo *ei)
{
	if (ac == 2) {
		if (chdir(av[1])) {
			warn("chdir: %s", av[1]);
			exitinfo_exit(ei, 1);
			return;
		}
		exitinfo_exit(ei, 0);
		return;
	}
	printf("Usage: chdir dir\n");
	exitinfo_exit(ei, 1);
}

/*
 * exit
 * pretty simple.  allow the user to choose the exit code if they want,
 * otherwise default to 0 (success).
 */
static
void
cmd_exit(int ac, char *av[], struct exitinfo *ei)
{
	int code;

	if (ac == 1) {
		code = 0;
	}
	else if (ac == 2) {
		code = atoi(av[1]);
	}
	else {
		printf("Usage: exit [code]\n");
		exitinfo_exit(ei, 1);
		return;
	}

	exit(code);
}

/*
 * a struct of the builtins associates the builtin name with the function that
 * executes it.  they must all take an argc and argv.
 */
static struct {
	const char *name;
	void (*func)(int, char **, struct exitinfo *);
} builtins[] = {
	{ "cd",    cmd_chdir },
	{ "chdir", cmd_chdir },
	{ "exit",  cmd_exit },
	{ "wait",  cmd_wait },
	{ NULL, NULL }
};

/*
 * docommand
 * tokenizes the command line using strtok.  if there aren't any commands,
 * simply returns.  checks to see if it's a builtin, running it if it is.
 * otherwise, it's a standard command.  check for the '&', try to background
 * the job if possible, otherwise just run it and wait on it.
 */
static
void
docommand(char *buf, struct exitinfo *ei)
{
	char *args[NARG_MAX + 1];
	int nargs, i;
	char *s;
	pid_t pid;
	int status;
	int bg=0;
	time_t startsecs, endsecs;
	unsigned long startnsecs, endnsecs;

	nargs = 0;
	for (s = strtok(buf, " \t\r\n"); s; s = strtok(NULL, " \t\r\n")) {
		if (nargs >= NARG_MAX) {
			printf("%s: Too many arguments "
			       "(exceeds system limit)\n",
			       args[0]);
			exitinfo_exit(ei, 1);
			return;
		}
		args[nargs++] = s;
	}
	args[nargs] = NULL;

	if (nargs==0) {
		/* empty line */
		exitinfo_exit(ei, 0);
		return;
	}

	for (i=0; builtins[i].name; i++) {
		if (!strcmp(builtins[i].name, args[0])) {
			builtins[i].func(nargs, args, ei);
			return;
		}
	}

	/* Not a builtin; run it */

	if (nargs > 0 && !strcmp(args[nargs-1], "&")) {
		/* background */
		if (!can_bg()) {
			printf("%s: Too many background jobs; wait for "
			       "some to finish before starting more\n",
			       args[0]);
			exitinfo_exit(ei, 1);
			return;
		}
		nargs--;
		args[nargs] = NULL;
		bg = 1;
	}

	if (timing) {
		__time(&startsecs, &startnsecs);
	}

	pid = fork();
	switch (pid) {
		case -1:
			/* error */
			warn("fork");
			exitinfo_exit(ei, 255);
			return;
		case 0:
			/* child */
			execvp(args[0], args);
			warn("%s", args[0]);
			/*
			 * Use _exit() instead of exit() in the child
			 * process to avoid calling atexit() functions,
			 * which would cause hostcompat (if present) to
			 * reset the tty state and mess up our input
			 * handling.
			 */
			_exit(1);
		default:
			break;
	}

	/* parent */
	if (bg) {
		/* background this command */
		remember_bg(pid);
		printf("[%d] %s ... &\n", pid, args[0]);
		exitinfo_exit(ei, 0);
		return;
	}

	if (waitpid(pid, &status, 0) < 0) {
		warn("waitpid");
		exitinfo_exit(ei, 255);
	}
	else {
		readstatus(status, ei);
	}

	if (timing) {
		__time(&endsecs, &endnsecs);
		if (endnsecs < startnsecs) {
			endnsecs += 1000000000;
			endsecs--;
		}
		endnsecs -= startnsecs;
		endsecs -= startsecs;
		warnx("subprocess time: %lu.%09lu seconds",
		      (unsigned long) endsecs, (unsigned long) endnsecs);
	}
}

/*
 * getcmd
 * pulls valid characters off the console, filling the buffer.
 * backspace deletes a character, simply by moving the position back.
 * a newline or carriage return breaks the loop, which terminates
 * the string and returns.
 *
 * if there's an invalid character or a backspace when there's nothing
 * in the buffer, putchars an alert (bell).
 */
static
void
getcmd(char *buf, size_t len)
{
	size_t pos = 0;
	int done=0, ch;

	/*
	 * In the absence of a <ctype.h>, assume input is 7-bit ASCII.
	 */

	while (!done) {
		ch = getchar();
		if ((ch == '\b' || ch == 127) && pos > 0) {
			putchar('\b');
			putchar(' ');
			putchar('\b');
			pos--;
		}
		else if (ch == '\r' || ch == '\n') {
			putchar('\r');
			putchar('\n');
			done = 1;
		}
		else if (ch >= 32 && ch < 127 && pos < len-1) {
			buf[pos++] = ch;
			putchar(ch);
		}
		else {
			/* alert (bell) character */
			putchar('\a');
		}
	}
	buf[pos] = 0;
}

/*
 * interactive
 * runs the interactive shell.  basically, just infinitely loops, grabbing
 * commands and running them (and printing the exit status if it's not
 * success.)
 */
static
void
interactive(void)
{
	char buf[CMDLINE_MAX];
	struct exitinfo ei;

	while (1) {
		printf("OS/161$ ");
		getcmd(buf, sizeof(buf));
		docommand(buf, &ei);
		printstatus(&ei, 0);
#ifdef WNOHANG
		waitpoll();
#endif
	}
}

static
void
check_timing(void)
{
	time_t secs;
	unsigned long nsecs;
	if (__time(&secs, &nsecs) != -1) {
		timing = 1;
		warnx("Timing enabled.");
	}
}

/*
 * main
 * if there are no arguments, run interactively, otherwise, run a program
 * from within the shell, but immediately exit.
 */
int
main(int argc, char *argv[])
{
#ifdef HOST
	hostcompat_init(argc, argv);
#endif
	check_timing();

	/*
	 * Allow argc to be 0 in case we're running on a broken kernel,
	 * or one that doesn't set argv when starting the first shell.
	 */
	if (argc == 0 || argc == 1) {
		interactive();
	}
	else if (argc == 3 && !strcmp(argv[1], "-c")) {
		struct exitinfo ei;
		docommand(argv[2], &ei);
		printstatus(&ei, 0);
		if (ei.signaled || ei.stopped || ei.val != 0) {
			exit(1);
		}
	}
	else {
		errx(1, "Usage: sh [-c command]");
	}
	return 0;
}
