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

#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "hostcompat.h"

/*
 * The program name.
 * This is used in err.c.
 */
const char *hostcompat_progname = NULL;

/*
 * Unix tty state, for when we're running and to put it back the way it was,
 * respectively.
 */
static struct termios hostcompat_runtios;
static struct termios hostcompat_savetios;

/*
 * Put the tty state back the way it was.
 */
static
void
hostcompat_ttyreset(void)
{
	tcsetattr(STDIN_FILENO, TCSADRAIN, &hostcompat_savetios);
}

/*
 * Set the tty state back to the way we want it for running.
 */
static
void
hostcompat_ttyresume(void)
{
	tcsetattr(STDIN_FILENO, TCSADRAIN, &hostcompat_runtios);
}

/*
 * Set up the tty state stuff.
 */
static
int
hostcompat_ttysetup(void)
{
	struct termios tios;

	/* Get the current tty state. */
	if (tcgetattr(STDIN_FILENO, &tios) < 0) {
		/* stdin is not a tty */
		return -1;
	}

	hostcompat_savetios = tios;

	/* Turn off canonical ("cooked") input. */
	tios.c_lflag &= ~ICANON;

	/*
	 * With canonical input off, this says how many characters must be
	 * typed before read() will return.
	 */
	tios.c_cc[VMIN] = 1;

	/* This can be used to set up read timeouts, but we don't need that. */
	tios.c_cc[VTIME] = 0;

	/* Turn off echoing of keypresses. */
	tios.c_lflag &= ~(ECHO|ECHONL|ECHOCTL);

	/* Do not support XON/XOFF flow control. */
	tios.c_iflag &= ~(IXON|IXOFF);

	/* On input, we want no CR/LF translation. */
	tios.c_iflag &= ~(INLCR|IGNCR|ICRNL);

	/* However, on output we want LF ('\n') mapped to CRLF. */
#ifdef OCRNL	/* missing on OS X */
	tios.c_oflag &= ~(OCRNL);
#endif
	tios.c_oflag |= OPOST|ONLCR;

	/* Enable keyboard signals (^C, ^Z, etc.) because they're useful. */
	tios.c_lflag |= ISIG;

	/* Set the new tty state. */
	hostcompat_runtios = tios;
	tcsetattr(STDIN_FILENO, TCSADRAIN, &tios);

	return 0;
}

/*
 * Signal handler for all the fatal signals (SIGSEGV, SIGTERM, etc.)
 */
static
void
hostcompat_die(int sig)
{
	/* Set the tty back to the way we found it */
	hostcompat_ttyreset();

	/* Make sure the default action will occur when we get another signal*/
	signal(sig, SIG_DFL);

	/* Post the signal back to ourselves, to cause the right exit status.*/
	kill(getpid(), sig);

	/* Just in case. */
	_exit(255);
}

/*
 * Signal handler for the stop signals (SIGTSTP, SIGTTIN, etc.)
 */
static
void
hostcompat_stop(int sig)
{
	/* Set the tty back to the way we found it */
	hostcompat_ttyreset();

	/* Make sure the default action will occur when we get another signal*/
	signal(sig, SIG_DFL);

	/* Post the signal back to ourselves. */
	kill(getpid(), sig);
}

/*
 * Signal handler for SIGCONT.
 */
static
void
hostcompat_cont(int sig)
{
	(void)sig;

	/* Set the tty to the way we want it for running. */
	hostcompat_ttyresume();

	/*
	 * Reload the signal handlers for stop/continue signals, in case
	 * they were set up with one-shot signals.
	 */
	signal(SIGTTIN, hostcompat_stop);
	signal(SIGTTOU, hostcompat_stop);
	signal(SIGTSTP, hostcompat_stop);
	signal(SIGCONT, hostcompat_cont);
}

/*
 * Initialize the hostcompat library.
 */
void
hostcompat_init(int argc, char *argv[])
{
	/* Set the program name */
	if (argc > 0 && argv[0] != NULL) {
		hostcompat_progname = argv[0];
	}

	/* Set the tty modes */
	if (hostcompat_ttysetup() < 0) {
		return;
	}

	/* When exit() is called, clean up */
	atexit(hostcompat_ttyreset);

	/* stdout/stderr should be unbuffered */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/* Catch all the fatal signals, so we can clean up */
	signal(SIGHUP, hostcompat_die);
	signal(SIGINT, hostcompat_die);
	signal(SIGQUIT, hostcompat_die);
	signal(SIGILL, hostcompat_die);
	signal(SIGTRAP, hostcompat_die);
	signal(SIGABRT, hostcompat_die);
#ifdef SIGEMT
	signal(SIGEMT, hostcompat_die);
#endif
	signal(SIGFPE, hostcompat_die);
	signal(SIGBUS, hostcompat_die);
	signal(SIGSEGV, hostcompat_die);
	signal(SIGSYS, hostcompat_die);
	signal(SIGPIPE, hostcompat_die);
	signal(SIGALRM, hostcompat_die);
	signal(SIGTERM, hostcompat_die);
	signal(SIGXCPU, hostcompat_die);
	signal(SIGXFSZ, hostcompat_die);
	signal(SIGVTALRM, hostcompat_die);
	signal(SIGPROF, hostcompat_die);
	signal(SIGUSR1, hostcompat_die);
	signal(SIGUSR2, hostcompat_die);

	/* Catch the stop signals, so we can adjust the tty */
	signal(SIGTTIN, hostcompat_stop);
	signal(SIGTTOU, hostcompat_stop);
	signal(SIGTSTP, hostcompat_stop);

	/* Catch the continue signal, so we can adjust the tty */
	signal(SIGCONT, hostcompat_cont);
}
