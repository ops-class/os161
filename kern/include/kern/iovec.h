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

#ifndef _KERN_IOVEC_H_
#define _KERN_IOVEC_H_

/*
 * iovec structure, used in the readv/writev scatter/gather I/O calls,
 * and within the kernel for keeping track of blocks of data for I/O.
 */

struct iovec {
	/*
	 * For maximum type safety, when in the kernel, distinguish
	 * user pointers from kernel pointers.
	 *
	 * (A pointer is a user pointer if it *came* from userspace,
	 * not necessarily if it *points* to userspace. If a system
	 * call passes 0xdeadbeef, it points to the kernel, but it's
	 * still a user pointer.)
	 *
	 * In userspace, there are only user pointers; also, the name
	 * iov_base is defined by POSIX.
	 *
	 * Note that to work properly (without extra unwanted fiddling
	 * around) this scheme requires that void* and userptr_t have
	 * the same machine representation. Machines where this isn't
	 * true are theoretically possible under the C standard, but
	 * do not exist in practice.
	 */
#ifdef _KERNEL
        union {
                userptr_t  iov_ubase;	/* user-supplied pointer */
                void      *iov_kbase;	/* kernel-supplied pointer */
        };
#else
	void *iov_base;			/* user-supplied pointer */
#endif
        size_t iov_len;			/* Length of data */
};

#endif /* _KERN_IOVEC_H_ */
