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

#ifndef _KERN_MIPS_TYPES_H_
#define _KERN_MIPS_TYPES_H_

/*
 * Machine-dependent types visible to userland.
 * (Kernel-only types should go in mips/types.h.)
 * 32-bit MIPS version.
 *
 * See kern/types.h for an explanation of the underscores.
 */


/* Sized integer types, with convenient short names */
typedef char      __i8;                 /* 8-bit signed integer */
typedef short     __i16;                /* 16-bit signed integer */
typedef int       __i32;                /* 32-bit signed integer */
typedef long long __i64;                /* 64-bit signed integer */

typedef unsigned char      __u8;        /* 8-bit unsigned integer */
typedef unsigned short     __u16;       /* 16-bit unsigned integer */
typedef unsigned int       __u32;       /* 32-bit unsigned integer */
typedef unsigned long long __u64;       /* 64-bit unsigned integer */

/* Further standard C types */
typedef long __intptr_t;                /* Signed pointer-sized integer */
typedef unsigned long __uintptr_t;      /* Unsigned pointer-sized integer */

/*
 * Since we're a 32-bit platform, size_t, ssize_t, and ptrdiff_t can
 * correctly be either (unsigned) int or (unsigned) long. However, if we
 * don't define it to the same one gcc is using, gcc will get
 * upset. If you switch compilers and see otherwise unexplicable type
 * errors involving size_t, try changing this.
 */
#if 1
typedef unsigned __size_t;              /* Size of a memory region */
typedef int __ssize_t;                  /* Signed type of same size */
typedef int __ptrdiff_t;                /* Difference of two pointers */
#else
typedef unsigned long __size_t;         /* Size of a memory region */
typedef long __ssize_t;                 /* Signed type of same size */
typedef long __ptrdiff_t;               /* Difference of two pointers */
#endif

/* Number of bits per byte. */
#define __CHAR_BIT  8


#endif /* _KERN_MIPS_TYPES_H_ */
