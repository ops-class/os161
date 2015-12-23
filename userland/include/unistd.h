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

#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <sys/cdefs.h>
#include <sys/types.h>

/*
 * Get the various constants (flags, codes, etc.) for calls from
 * kernel includes. This way user-level code doesn't need to know
 * about the kern/ headers.
 */
#include <kern/fcntl.h>
#include <kern/ioctl.h>
#include <kern/reboot.h>
#include <kern/seek.h>
#include <kern/time.h>
#include <kern/unistd.h>
#include <kern/wait.h>


/*
 * Prototypes for OS/161 system calls.
 *
 * Note that the following system calls are prototyped in other
 * header files, as follows:
 *
 *     stat:     sys/stat.h
 *     fstat:    sys/stat.h
 *     lstat:    sys/stat.h
 *     mkdir:    sys/stat.h
 *
 * If this were standard Unix, more prototypes would go in other
 * header files as well, as follows:
 *
 *     waitpid:  sys/wait.h
 *     open:     fcntl.h or sys/fcntl.h
 *     reboot:   sys/reboot.h
 *     ioctl:    sys/ioctl.h
 *     remove:   stdio.h
 *     rename:   stdio.h
 *     time:     time.h
 *
 * Also note that the prototypes for open() and mkdir() contain, for
 * compatibility with Unix, an extra argument that is not meaningful
 * in OS/161. This is the "mode" (file permissions) for a newly created
 * object. (With open, if no file is created, this is ignored, and the
 * call prototype is gimmicked so it doesn't have to be passed either.)
 *
 * You should ignore these arguments in the OS/161 kernel unless you're
 * implementing security and file permissions.
 *
 * If you are implementing security and file permissions and using a
 * model different from Unix so that you need different arguments to
 * these calls, you may make appropriate changes, or define new syscalls
 * with different names and take the old ones out, or whatever.
 *
 * As a general rule of thumb, however, while you can make as many new
 * syscalls of your own as you like, you shouldn't change the
 * definitions of the ones that are already here. They've been written
 * to be pretty much compatible with Unix, and the teaching staff has
 * test code that expects them to behave in particular ways.
 *
 * Of course, if you want to redesign the user/kernel API and make a
 * lot of work for yourself, feel free, just contact the teaching
 * staff beforehand. :-)
 *
 * The categories (required/recommended/optional) are guesses - check
 * the text of the various assignments for an authoritative list.
 */


/*
 * NOTE NOTE NOTE NOTE NOTE
 *
 * This file is *not* shared with the kernel, even though in a sense
 * the kernel needs to know about these prototypes. This is because,
 * due to error handling concerns, the in-kernel versions of these
 * functions will usually have slightly different signatures.
 */


/* Required. */
__DEAD void _exit(int code);
int execv(const char *prog, char *const *args);
pid_t fork(void);
pid_t waitpid(pid_t pid, int *returncode, int flags);
/*
 * Open actually takes either two or three args: the optional third
 * arg is the file mode used for creation. Unless you're implementing
 * security and permissions, you can ignore it.
 */
int open(const char *filename, int flags, ...);
ssize_t read(int filehandle, void *buf, size_t size);
ssize_t write(int filehandle, const void *buf, size_t size);
int close(int filehandle);
int reboot(int code);
int sync(void);
/* mkdir - see sys/stat.h */
int rmdir(const char *dirname);

/* Recommended. */
pid_t getpid(void);
int ioctl(int filehandle, int code, void *buf);
off_t lseek(int filehandle, off_t pos, int code);
int fsync(int filehandle);
int ftruncate(int filehandle, off_t size);
int remove(const char *filename);
int rename(const char *oldfile, const char *newfile);
int link(const char *oldfile, const char *newfile);
/* fstat - see sys/stat.h */
int chdir(const char *path);

/* Optional. */
void *sbrk(__intptr_t change);
ssize_t getdirentry(int filehandle, char *buf, size_t buflen);
int symlink(const char *target, const char *linkname);
ssize_t readlink(const char *path, char *buf, size_t buflen);
int dup2(int filehandle, int newhandle);
int pipe(int filehandles[2]);
int __time(time_t *seconds, unsigned long *nanoseconds);
ssize_t __getcwd(char *buf, size_t buflen);
/* stat - see sys/stat.h */
/* lstat - see sys/stat.h */

/*
 * These are not themselves system calls, but wrapper routines in libc.
 */

int execvp(const char *prog, char *const *args); /* calls execv */
char *getcwd(char *buf, size_t buflen);		/* calls __getcwd */
time_t time(time_t *seconds);			/* calls __time */

#endif /* _UNISTD_H_ */
