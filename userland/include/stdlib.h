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

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <kern/types.h>
#include <types/size_t.h>
#include <sys/null.h>

/*
 * Ascii to integer - turn a string holding a number into a number.
 */
int atoi(const char *);

/*
 * Standard routine to bail out of a program in a severe error condition.
 */
void abort(void);

/*
 * Routine to exit cleanly.
 * (This does libc cleanup before calling the _exit system call.)
 */
void exit(int code);

/*
 * Get the value of an environment variable. A default environment is
 * provided if the kernel doesn't pass environment strings.
 */
char *getenv(const char *var);

/*
 * Run a command. Returns its exit status as it comes back from waitpid().
 */
int system(const char *command);

/*
 * Pseudo-random number generator.
 */
#define RAND_MAX  0x7fffffff
long random(void);
void srandom(unsigned long seed);
char *initstate(unsigned long, char *, size_t);
char *setstate(char *);

/*
 * Memory allocation functions.
 */
void *malloc(size_t size);
void free(void *ptr);

/*
 * Sort.
 */
void qsort(void *data, unsigned num, size_t size,
	   int (*f)(const void *, const void *));

#endif /* _STDLIB_H_ */
