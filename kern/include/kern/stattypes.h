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

#ifndef _KERN_STATTYPES_H_
#define _KERN_STATTYPES_H_

/*
 * Further supporting material for stat(), fstat(), and lstat().
 *
 * File types for st_mode. (The permissions are the low 12 bits.)
 *
 * These are also used, shifted right by those 12 bits, in struct
 * dirent in libc, which is why they get their own file.
 *
 * Non-underscore versions of the names can be gotten from <stat.h>
 * (kernel) or <sys/stat.h> (userland).
 */

#define _S_IFMT   070000	/* mask for type of file */
#define _S_IFREG  010000	/* ordinary regular file */
#define _S_IFDIR  020000	/* directory */
#define _S_IFLNK  030000	/* symbolic link */
#define _S_IFIFO  040000	/* pipe or named pipe */
#define _S_IFSOCK 050000	/* socket */
#define _S_IFCHR  060000	/* character device */
#define _S_IFBLK  070000	/* block device */


#endif /* _KERN_STATTYPES_H_ */
