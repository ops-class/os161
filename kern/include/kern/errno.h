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

#ifndef _KERN_ERRNO_H_
#define _KERN_ERRNO_H_

/*
 * If you change this, be sure to make appropriate corresponding changes
 * to kern/errmsg.h as well. You might also want to change the man page
 * for errno to document the new error.
 *
 * This has been changed relative to OS/161 1.x to make the grouping
 * more logical.
 *
 * Also note that this file has to work from assembler, so it should
 * contain only symbolic constants.
 */

#define ENOSYS          1      /* Function not implemented */
/* unused               2                                  */
#define ENOMEM          3      /* Out of memory */
#define EAGAIN          4      /* Operation would block */
#define EINTR           5      /* Interrupted system call */
#define EFAULT          6      /* Bad memory reference */
#define ENAMETOOLONG    7      /* String too long */
#define EINVAL          8      /* Invalid argument */
#define EPERM           9      /* Operation not permitted */
#define EACCES          10     /* Permission denied */
#define EMPROC          11     /* Too many processes */
#define ENPROC          12     /* Too many processes in system */
#define ENOEXEC         13     /* File is not executable */
#define E2BIG           14     /* Argument list too long */
#define ESRCH           15     /* No such process */
#define ECHILD          16     /* No child processes */
#define ENOTDIR         17     /* Not a directory */
#define EISDIR          18     /* Is a directory */
#define ENOENT          19     /* No such file or directory */
#define ELOOP           20     /* Too many levels of symbolic links */
#define ENOTEMPTY       21     /* Directory not empty */
#define EEXIST          22     /* File or object exists */
#define EMLINK          23     /* Too many hard links */
#define EXDEV           24     /* Cross-device link */
#define ENODEV          25     /* No such device */
#define ENXIO           26     /* Device not available */
#define EBUSY           27     /* Device or resource busy */
#define EMFILE          28     /* Too many open files */
#define ENFILE          29     /* Too many open files in system */
#define EBADF           30     /* Bad file number */
#define EIOCTL          31     /* Invalid or inappropriate ioctl */
#define EIO             32     /* Input/output error */
#define ESPIPE          33     /* Illegal seek */
#define EPIPE           34     /* Broken pipe */
#define EROFS           35     /* Read-only file system */
#define ENOSPC          36     /* No space left on device */
#define EDQUOT          37     /* Disc quota exceeded */
#define EFBIG           38     /* File too large */
#define EFTYPE          39     /* Invalid file type or format */
#define EDOM            40     /* Argument out of range */
#define ERANGE          41     /* Result out of range */
#define EILSEQ          42     /* Invalid multibyte character sequence */
#define ENOTSOCK        43     /* Not a socket */
#define EISSOCK         44     /* Is a socket */
#define EISCONN         45     /* Socket is already connected */
#define ENOTCONN        46     /* Socket is not connected */
#define ESHUTDOWN       47     /* Socket has been shut down */
#define EPFNOSUPPORT    48     /* Protocol family not supported */
#define ESOCKTNOSUPPORT 49     /* Socket type not supported */
#define EPROTONOSUPPORT 50     /* Protocol not supported */
#define EPROTOTYPE      51     /* Protocol wrong type for socket */
#define EAFNOSUPPORT   52 /* Address family not supported by protocol family */
#define ENOPROTOOPT     53     /* Protocol option not available */
#define EADDRINUSE      54     /* Address already in use */
#define EADDRNOTAVAIL   55     /* Cannot assign requested address */
#define ENETDOWN        56     /* Network is down */
#define ENETUNREACH     57     /* Network is unreachable */
#define EHOSTDOWN       58     /* Host is down */
#define EHOSTUNREACH    59     /* Host is unreachable */
#define ECONNREFUSED    60     /* Connection refused */
#define ETIMEDOUT       61     /* Connection timed out */
#define ECONNRESET      62     /* Connection reset by peer */
#define EMSGSIZE        63     /* Message too large */
#define ENOTSUP         64     /* Threads operation not supported */


#endif /* _KERN_ERRNO_H_ */
