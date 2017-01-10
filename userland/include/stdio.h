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

#ifndef _STDIO_H_
#define _STDIO_H_

/*
 * Get the __-protected definition of va_list. We aren't supposed to
 * include stdarg.h here.
 */
#include <kern/types.h>
#include <types/size_t.h>
#include <sys/null.h>

/* Constant returned by a bunch of stdio functions on error */
#define EOF (-1)

/*
 * The actual guts of printf
 * (for libc internal use only)
 */
int __vprintf(void (*sendfunc)(void *clientdata, const char *, size_t len),
	      void *clientdata,
	      const char *fmt,
	      __va_list ap);

/* Printf calls for user programs */
int printf(const char *fmt, ...);
int vprintf(const char *fmt, __va_list ap);
int snprintf(char *buf, size_t len, const char *fmt, ...);
int vsnprintf(char *buf, size_t len, const char *fmt, __va_list ap);

/* Print the argument string and then a newline. Returns 0 or -1 on error. */
int puts(const char *);

/* Like puts, but without the newline. Returns #chars written. */
/* Nonstandard C, hence the __. */
int __puts(const char *);

/* Writes one character. Returns it. */
int putchar(int);

/* Reads one character (0-255) or returns EOF on error. */
int getchar(void);

#endif /* _STDIO_H_ */
