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

#ifndef _KERN_LIMITS_H_
#define _KERN_LIMITS_H_

/*
 * Constants for libc's <limits.h> - system limits.
 *
 * The symbols are prefixed with __ here to avoid namespace pollution
 * in libc. Use <limits.h> (in either userspace or the kernel) to get
 * the proper names.
 *
 * These are Unix-style limits that Unix defines; you can change them
 * around or add others as needed or as are appropriate to your system
 * design.
 *
 * Likewise, the default values provided here are fairly reasonable,
 * but you can change them around pretty freely and userspace code
 * should adapt. Do change these as needed to match your
 * implementation.
 */


/*
 * Important, both as part of the system call API and for system behavior.
 *
 * 255 for NAME_MAX and 1024 for PATH_MAX are conventional. ARG_MAX
 * should be at least 16K. In real systems it often runs to 256K or
 * more.
 */

/* Longest filename (without directory) not including null terminator */
#define __NAME_MAX      255

/* Longest full path name */
#define __PATH_MAX      1024

/* Max bytes for an exec function (should be at least 16K) */
#define __ARG_MAX       (64 * 1024)


/*
 * Important for system behavior, but not a big part of the API.
 *
 * Most modern systems don't have OPEN_MAX at all, and instead go by
 * whatever limit is set with setrlimit().
 */

/* Min value for a process ID (that can be assigned to a user process) */
#define __PID_MIN       2

/* Max value for a process ID (change this to match your implementation) */
#define __PID_MAX       32767

/* Max open files per process */
#define __OPEN_MAX      128

/* Max bytes for atomic pipe I/O -- see description in the pipe() man page */
#define __PIPE_BUF      512


/*
 * Not so important parts of the API. (Especially in OS/161 where we
 * don't do credentials by default.)
 */

/* Max number of supplemental group IDs in process credentials */
#define __NGROUPS_MAX   32

/* Max login name size (for setlogin/getlogin), incl. null */
#define __LOGIN_NAME_MAX 17


/*
 * Not very important at all.
 */

/* Max number of iovec structures at once for readv/writev/preadv/pwritev */
#define __IOV_MAX       1024


#endif /* _KERN_LIMITS_H_ */
