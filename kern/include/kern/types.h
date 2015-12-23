/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008
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

#ifndef _KERN_TYPES_H_
#define _KERN_TYPES_H_

/* Get machine-dependent types. */
#include <kern/machine/types.h>

/*
 * Machine-independent types visible to user level.
 *
 * Define everything with leading underscores to avoid polluting the C
 * namespace for applications.
 *
 * The C standard (and additionally the POSIX standard) define rules
 * for what families of symbol names are allowed to be used by
 * application programmers, and what families of symbol names can be
 * defined by various standard header files. The C library needs to
 * conform to those rules, to the extent reasonably practical, to make
 * sure that application code compiles and behaves as intended.
 *
 * Many of the C library's headers need to use one or more of these
 * types in places where the "real" name of the type cannot be
 * exposed, or expose the names of some of these types and not others.
 * (For example, <string.h> is supposed to define size_t, but is not
 * supposed to also define e.g. pid_t.)
 *
 * For this reason we define everything with two underscores in front
 * of it; in C such symbol names are reserved for the implementation,
 * which we are, so this file can be included anywhere in any libc
 * header without causing namespace problems. The "real" type names
 * are defined with an additional layer of typedefs; this happens for
 * the kernel in <types.h> and for userland in (mostly) <sys/types.h>
 * and also various other places as per relevant standards.
 */

typedef __u32 __blkcnt_t;  /* Count of blocks */
typedef __u32 __blksize_t; /* Size of an I/O block */
typedef __u64 __counter_t; /* Event counter */
typedef __u32 __daddr_t;   /* Disk block number */
typedef __u32 __dev_t;     /* Hardware device ID */
typedef __u32 __fsid_t;    /* Filesystem ID */
typedef __i32 __gid_t;     /* Group ID */
typedef __u32 __in_addr_t; /* Internet address */
typedef __u32 __in_port_t; /* Internet port number */
typedef __u32 __ino_t;     /* Inode number */
typedef __u32 __mode_t;    /* File access mode */
typedef __u16 __nlink_t;   /* Number of links (intentionally only 16 bits) */
typedef __i64 __off_t;     /* Offset within file */
typedef __i32 __pid_t;     /* Process ID */
typedef __u64 __rlim_t;    /* Resource limit quantity */
typedef __u8 __sa_family_t;/* Socket address family */
typedef __i64 __time_t;    /* Time in seconds */
typedef __i32 __uid_t;     /* User ID */

typedef int __nfds_t;    /* Number of file handles */
typedef int __socklen_t;   /* Socket-related length */

/* See note in <stdarg.h> */
#ifdef __GNUC__
typedef __builtin_va_list __va_list;
#endif


#endif /* _KERN_TYPES_H_ */
