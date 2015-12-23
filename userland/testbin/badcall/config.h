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
 * Note: if the assumptions in the constants below are violated by
 * your system design, please change the values as necessary. Don't
 * change stuff in the .c files, or disable tests, without consulting
 * the course staff first.
 */

#if defined(__mips__)
#define KERN_PTR	((void *)0x80000000)	/* addr within kernel */
#define INVAL_PTR	((void *)0x40000000)	/* addr not part of program */
#else
#error "Please fix this"
#endif

/*
 * We assume CLOSED_FD is a legal fd that won't be open when we're running.
 * CLOSED_FD+1 should also be legal and not open.
 */
#define CLOSED_FD		10

/* We assume IMPOSSIBLE_FD is a fd that is completely not allowed. */
#define IMPOSSIBLE_FD		1234567890

/* We assume this pid won't exist while we're running. Change as needed. */
#define NONEXIST_PID		34000

/* An arbitrary process exit code that hopefully won't occur by accident */
#define MAGIC_STATUS		107

/* An ioctl that doesn't exist */
#define NONEXIST_IOCTL		12345
