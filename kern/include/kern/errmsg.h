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

#ifndef _KERN_ERRMSG_H_
#define _KERN_ERRMSG_H_

/*
 * Error strings.
 * This table must agree with kern/errno.h.
 *
 * Note that since this actually defines sys_errlist and sys_nerrlist, it
 * should only be included in one file. For the kernel, that file is
 * lib/misc.c; for userland it's lib/libc/strerror.c.
 */
const char *const sys_errlist[] = {
	"Operation succeeded",        /* 0 */
	"Function not implemented",   /* ENOSYS */
	"(undefined error 2)",        /* unused */
	"Out of memory",              /* ENOMEM */
	"Operation would block",      /* EAGAIN (also EWOULDBLOCK) */
	"Interrupted system call",    /* EINTR */
	"Bad memory reference",       /* EFAULT */
	"String too long",            /* ENAMETOOLONG */
	"Invalid argument",           /* EINVAL */
	"Operation not permitted",    /* EPERM */
	"Permission denied",          /* EACCES */
	"Too many processes",         /* EMPROC (EPROCLIM in Unix) */
	"Too many processes in system",/* ENPROC */
	"File is not executable",     /* ENOEXEC */
	"Argument list too long",     /* E2BIG */
	"No such process",            /* ESRCH */
	"No child processes",         /* ECHILD */
	"Not a directory",            /* ENOTDIR */
	"Is a directory",             /* EISDIR */
	"No such file or directory",  /* ENOENT */
	"Too many levels of symbolic links",/* ELOOP */
	"Directory not empty",        /* ENOTEMPTY */
	"File or object exists",      /* EEXIST */
	"Too many hard links",        /* EMLINK */
	"Cross-device link",          /* EXDEV */
	"No such device",             /* ENODEV */
	"Device not available",       /* ENXIO */
	"Device or resource busy",    /* EBUSY */
	"Too many open files",        /* EMFILE */
	"Too many open files in system",/* ENFILE */
	"Bad file number",            /* EBADF */
	"Invalid or inappropriate ioctl",/* EIOCTL (ENOTTY in Unix) */
	"Input/output error",         /* EIO */
	"Illegal seek",               /* ESPIPE */
	"Broken pipe",                /* EPIPE */
	"Read-only file system",      /* EROFS */
	"No space left on device",    /* ENOSPC */
	"Disc quota exceeded",        /* EDQUOT */
	"File too large",             /* EFBIG */
	"Invalid file type or format",/* EFTYPE */
	"Argument out of range",      /* EDOM */
	"Result out of range",        /* ERANGE */
	"Invalid multibyte character sequence",/* EILSEQ */
	"Not a socket",               /* ENOTSOCK */
	"Is a socket",                /* EISSOCK (EOPNOTSUPP in Unix) */
	"Socket is already connected",/* EISCONN */
	"Socket is not connected",    /* ENOTCONN */
	"Socket has been shut down",  /* ESHUTDOWN */
	"Protocol family not supported",/* EPFNOSUPPORT */
	"Socket type not supported",  /* ESOCKTNOSUPPORT */
	"Protocol not supported",     /* EPROTONOSUPPORT */
	"Protocol wrong type for socket",/* EPROTOTYPE */
	"Address family not supported by protocol family",/* EAFNOSUPPORT */
	"Protocol option not available",/* ENOPROTOOPT */
	"Address already in use",     /* EADDRINUSE */
	"Cannot assign requested address",/* EADDRNOTAVAIL */
	"Network is down",            /* ENETDOWN */
	"Network is unreachable",     /* ENETUNREACH */
	"Host is down",               /* EHOSTDOWN */
	"Host is unreachable",        /* EHOSTUNREACH */
	"Connection refused",         /* ECONNREFUSED */
	"Connection timed out",       /* ETIMEDOUT */
	"Connection reset by peer",   /* ECONNRESET */
	"Message too large",          /* EMSGSIZE */
	"Threads operation not supported",/* ENOTSUP */
};

/*
 * Number of entries in sys_errlist.
 */
const int sys_nerr = sizeof(sys_errlist)/sizeof(const char *);

#endif /* _KERN_ERRMSG_H_ */
