/*
 * Copyright (c) 2015
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

#ifndef HANGMAN_H
#define HANGMAN_H

/*
 * Simple deadlock detector. Enable with "options hangman" in the
 * kernel config.
 */

#include "opt-hangman.h"

#if OPT_HANGMAN

struct hangman_actor {
	const char *a_name;
	const struct hangman_lockable *a_waiting;
};

struct hangman_lockable {
	const char *l_name;
	const struct hangman_actor *l_holding;
};

void hangman_wait(struct hangman_actor *a, struct hangman_lockable *l);
void hangman_acquire(struct hangman_actor *a, struct hangman_lockable *l);
void hangman_release(struct hangman_actor *a, struct hangman_lockable *l);

#define HANGMAN_ACTOR(sym)	struct hangman_actor sym
#define HANGMAN_LOCKABLE(sym)	struct hangman_lockable sym

#define HANGMAN_ACTORINIT(a, n)	    ((a)->a_name = (n), (a)->a_waiting = NULL)
#define HANGMAN_LOCKABLEINIT(l, n)  ((l)->l_name = (n), (l)->l_holding = NULL)

#define HANGMAN_LOCKABLE_INITIALIZER	{ "spinlock", NULL }

#define HANGMAN_WAIT(a, l)	hangman_wait(a, l)
#define HANGMAN_ACQUIRE(a, l)	hangman_acquire(a, l)
#define HANGMAN_RELEASE(a, l)	hangman_release(a, l)

#else

#define HANGMAN_ACTOR(sym)
#define HANGMAN_LOCKABLE(sym)

#define HANGMAN_ACTORINIT(a, name)
#define HANGMAN_LOCKABLEINIT(a, name)

#define HANGMAN_LOCKABLE_INITIALIZER

#define HANGMAN_WAIT(a, l)
#define HANGMAN_ACQUIRE(a, l)
#define HANGMAN_RELEASE(a, l)

#endif

#endif /* HANGMAN_H */
