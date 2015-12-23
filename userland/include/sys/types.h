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

#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

/*
 * This header file is supposed to define standard system types,
 * stuff like size_t and pid_t, as well as define a few other
 * standard symbols like NULL.
 *
 * There are no such types that are user-level only.
 */

/* Get the exported kernel definitions, protected with __ */
#include <kern/types.h>

/* Pick up stuff that needs to be defined individually due to standards. */
#include <types/size_t.h>
#include <sys/null.h>

/*
 * Define the rest with user-visible names.
 *
 * Note that the standards-compliance stuff is not by any means
 * complete here yet...
 */

typedef __ssize_t ssize_t;
typedef __ptrdiff_t ptrdiff_t;

/* ...and machine-independent from <kern/types.h>. */
typedef __blkcnt_t blkcnt_t;
typedef __blksize_t blksize_t;
typedef __daddr_t daddr_t;
typedef __dev_t dev_t;
typedef __fsid_t fsid_t;
typedef __gid_t gid_t;
typedef __in_addr_t in_addr_t;
typedef __in_port_t in_port_t;
typedef __ino_t ino_t;
typedef __mode_t mode_t;
typedef __nlink_t nlink_t;
typedef __off_t off_t;
typedef __pid_t pid_t;
typedef __rlim_t rlim_t;
typedef __sa_family_t sa_family_t;
typedef __time_t time_t;
typedef __uid_t uid_t;

typedef __nfds_t nfds_t;
typedef __socklen_t socklen_t;

/*
 * Number of bits per byte.
 */

#define CHAR_BIT __CHAR_BIT


#endif /* _SYS_TYPES_H_ */
