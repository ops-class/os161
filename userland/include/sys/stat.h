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

#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

/*
 * Get struct stat and all the #defines from the kernel
 */
#include <kern/stat.h>
#include <kern/stattypes.h>

/*
 * Test macros for object types.
 */
#define S_ISDIR(mode) ((mode & _S_IFMT) == _S_IFDIR)
#define S_ISREG(mode) ((mode & _S_IFMT) == _S_IFREG)
#define S_ISDIR(mode) ((mode & _S_IFMT) == _S_IFDIR)
#define S_ISLNK(mode) ((mode & _S_IFMT) == _S_IFLNK)
#define S_ISIFO(mode) ((mode & _S_IFMT) == _S_IFIFO)
#define S_ISSOCK(mode) ((mode & _S_IFMT) ==_S_IFSOCK)
#define S_ISCHR(mode) ((mode & _S_IFMT) == _S_IFCHR)
#define S_ISBLK(mode) ((mode & _S_IFMT) == _S_IFBLK)

/*
 * Provide non-underscore names. These are not actually standard; for
 * some reason only the test macros are.
 */
#define S_IFMT   _S_IFMT
#define S_IFREG  _S_IFREG
#define S_IFDIR  _S_IFDIR
#define S_IFLNK  _S_IFLNK
#define S_IFIFO  _S_IFIFO
#define S_IFSOCK _S_IFSOCK
#define S_IFCHR  _S_IFCHR
#define S_IFBLK  _S_IFBLK

/*
 * stat is the same as fstat, only on a file that isn't already
 * open. lstat is the same as stat, only if the name passed names a
 * symlink, information about the symlink is returned rather than
 * information about the file it points to. You don't need to
 * implement lstat unless you're implementing symbolic links.
 */
int fstat(int filehandle, struct stat *buf);
int stat(const char *path, struct stat *buf);
int lstat(const char *path, struct stat *buf);

/*
 * The second argument to mkdir is the mode for the new directory.
 * Unless you're implementing security and permissions, you can
 * (and should) ignore it. See notes in unistd.h.
 */
int mkdir(const char *dirname, int ignore);


#endif /* _SYS_STAT_H_ */
