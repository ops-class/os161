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

#ifndef _COPYINOUT_H_
#define _COPYINOUT_H_


/*
 * copyin/copyout/copyinstr/copyoutstr are standard BSD kernel functions.
 *
 * copyin copies LEN bytes from a user-space address USERSRC to a
 * kernel-space address DEST.
 *
 * copyout copies LEN bytes from a kernel-space address SRC to a
 * user-space address USERDEST.
 *
 * copyinstr copies a null-terminated string of at most LEN bytes from
 * a user-space address USERSRC to a kernel-space address DEST, and
 * returns the actual length of string found in GOT. DEST is always
 * null-terminated on success. LEN and GOT include the null terminator.
 *
 * copyoutstr copies a null-terminated string of at most LEN bytes from
 * a kernel-space address SRC to a user-space address USERDEST, and
 * returns the actual length of string found in GOT. DEST is always
 * null-terminated on success. LEN and GOT include the null terminator.
 *
 * All of these functions return 0 on success, EFAULT if a memory
 * addressing error was encountered, or (for the string versions)
 * ENAMETOOLONG if the space available was insufficient.
 *
 * NOTE that the order of the arguments is the same as bcopy() or
 * cp/mv, that is, source on the left, NOT the same as strcpy().
 * The const qualifiers and types will help protect against mistakes
 * in this regard but are obviously not foolproof.
 *
 * These functions are machine-dependent; however, a common version
 * that can be used by a number of machine types is found in
 * vm/copyinout.c.
 */

int copyin(const_userptr_t usersrc, void *dest, size_t len);
int copyout(const void *src, userptr_t userdest, size_t len);
int copyinstr(const_userptr_t usersrc, char *dest, size_t len, size_t *got);
int copyoutstr(const char *src, userptr_t userdest, size_t len, size_t *got);


#endif /* _COPYINOUT_H_ */
