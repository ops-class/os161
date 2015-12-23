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

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <setjmp.h>
#include <thread.h>
#include <current.h>
#include <vm.h>
#include <copyinout.h>

/*
 * User/kernel memory copying functions.
 *
 * These are arranged to prevent fatal kernel memory faults if invalid
 * addresses are supplied by user-level code. This code is itself
 * machine-independent; it uses the machine-dependent C setjmp/longjmp
 * facility to perform recovery.
 *
 * However, it assumes things about the memory subsystem that may not
 * be true on all platforms.
 *
 * (1) It assumes that user memory is mapped into the current address
 * space while running in the kernel, and can be accessed by just
 * dereferencing a pointer in the ordinary way. (And not, for example,
 * with special instructions or via special segment registers.)
 *
 * (2) It assumes that the user-space region of memory is contiguous
 * and extends from 0 to some virtual address USERSPACETOP, and so if
 * a user process passes a kernel address the logic in copycheck()
 * will trap it.
 *
 * (3) It assumes that access to user memory from the kernel behaves
 * the same way as access to user memory from user space: for
 * instance, that the processor honors read-only bits on memory pages
 * when in kernel mode.
 *
 * (4) It assumes that if a proper user-space address that is valid
 * but not present, or not valid at all, is touched from the kernel,
 * that the correct faults will occur and the VM system will load the
 * necessary pages and whatnot.
 *
 * (5) It assumes that the machine-dependent trap logic provides and
 * honors a tm_badfaultfunc field in the thread_machdep structure.
 * This feature works as follows: if an otherwise fatal fault occurs
 * in kernel mode, and tm_badfaultfunc is set, execution resumes in
 * the function pointed to by tm_badfaultfunc.
 *
 * This code works by setting tm_badfaultfunc and then copying memory
 * in an ordinary fashion. If these five assumptions are satisfied,
 * which is the case for many ordinary CPU types, this code should
 * function correctly. If the assumptions are not satisfied on some
 * platform (for instance, certain old 80386 processors violate
 * assumption 3), this code cannot be used, and cpu- or platform-
 * specific code must be written.
 *
 * To make use of this code, in addition to tm_badfaultfunc the
 * thread_machdep structure should contain a jmp_buf called
 * "tm_copyjmp".
 */

/*
 * Recovery function. If a fatal fault occurs during copyin, copyout,
 * copyinstr, or copyoutstr, execution resumes here. (This behavior is
 * caused by setting t_machdep.tm_badfaultfunc and is implemented in
 * machine-dependent code.)
 *
 * We use the C standard function longjmp() to teleport up the call
 * stack to where setjmp() was called. At that point we return EFAULT.
 */
static
void
copyfail(void)
{
	longjmp(curthread->t_machdep.tm_copyjmp, 1);
}

/*
 * Memory region check function. This checks to make sure the block of
 * user memory provided (an address and a length) falls within the
 * proper userspace region. If it does not, EFAULT is returned.
 *
 * stoplen is set to the actual maximum length that can be copied.
 * This differs from len if and only if the region partially overlaps
 * the kernel.
 *
 * Assumes userspace runs from 0 through USERSPACETOP-1.
 */
static
int
copycheck(const_userptr_t userptr, size_t len, size_t *stoplen)
{
	vaddr_t bot, top;

	*stoplen = len;

	bot = (vaddr_t) userptr;
	top = bot+len-1;

	if (top < bot) {
		/* addresses wrapped around */
		return EFAULT;
	}

	if (bot >= USERSPACETOP) {
		/* region is within the kernel */
		return EFAULT;
	}

	if (top >= USERSPACETOP) {
		/* region overlaps the kernel. adjust the max length. */
		*stoplen = USERSPACETOP - bot;
	}

	return 0;
}

/*
 * copyin
 *
 * Copy a block of memory of length LEN from user-level address USERSRC
 * to kernel address DEST. We can use memcpy because it's protected by
 * the tm_badfaultfunc/copyfail logic.
 */
int
copyin(const_userptr_t usersrc, void *dest, size_t len)
{
	int result;
	size_t stoplen;

	result = copycheck(usersrc, len, &stoplen);
	if (result) {
		return result;
	}
	if (stoplen != len) {
		/* Single block, can't legally truncate it. */
		return EFAULT;
	}

	curthread->t_machdep.tm_badfaultfunc = copyfail;

	result = setjmp(curthread->t_machdep.tm_copyjmp);
	if (result) {
		curthread->t_machdep.tm_badfaultfunc = NULL;
		return EFAULT;
	}

	memcpy(dest, (const void *)usersrc, len);

	curthread->t_machdep.tm_badfaultfunc = NULL;
	return 0;
}

/*
 * copyout
 *
 * Copy a block of memory of length LEN from kernel address SRC to
 * user-level address USERDEST. We can use memcpy because it's
 * protected by the tm_badfaultfunc/copyfail logic.
 */
int
copyout(const void *src, userptr_t userdest, size_t len)
{
	int result;
	size_t stoplen;

	result = copycheck(userdest, len, &stoplen);
	if (result) {
		return result;
	}
	if (stoplen != len) {
		/* Single block, can't legally truncate it. */
		return EFAULT;
	}

	curthread->t_machdep.tm_badfaultfunc = copyfail;

	result = setjmp(curthread->t_machdep.tm_copyjmp);
	if (result) {
		curthread->t_machdep.tm_badfaultfunc = NULL;
		return EFAULT;
	}

	memcpy((void *)userdest, src, len);

	curthread->t_machdep.tm_badfaultfunc = NULL;
	return 0;
}

/*
 * Common string copying function that behaves the way that's desired
 * for copyinstr and copyoutstr.
 *
 * Copies a null-terminated string of maximum length MAXLEN from SRC
 * to DEST. If GOTLEN is not null, store the actual length found
 * there. Both lengths include the null-terminator. If the string
 * exceeds the available length, the call fails and returns
 * ENAMETOOLONG.
 *
 * STOPLEN is like MAXLEN but is assumed to have come from copycheck.
 * If we hit MAXLEN it's because the string is too long to fit; if we
 * hit STOPLEN it's because the string has run into the end of
 * userspace. Thus in the latter case we return EFAULT, not
 * ENAMETOOLONG.
 */
static
int
copystr(char *dest, const char *src, size_t maxlen, size_t stoplen,
	size_t *gotlen)
{
	size_t i;

	for (i=0; i<maxlen && i<stoplen; i++) {
		dest[i] = src[i];
		if (src[i] == 0) {
			if (gotlen != NULL) {
				*gotlen = i+1;
			}
			return 0;
		}
	}
	if (stoplen < maxlen) {
		/* ran into user-kernel boundary */
		return EFAULT;
	}
	/* otherwise just ran out of space */
	return ENAMETOOLONG;
}

/*
 * copyinstr
 *
 * Copy a string from user-level address USERSRC to kernel address
 * DEST, as per copystr above. Uses the tm_badfaultfunc/copyfail
 * logic to protect against invalid addresses supplied by a user
 * process.
 */
int
copyinstr(const_userptr_t usersrc, char *dest, size_t len, size_t *actual)
{
	int result;
	size_t stoplen;

	result = copycheck(usersrc, len, &stoplen);
	if (result) {
		return result;
	}

	curthread->t_machdep.tm_badfaultfunc = copyfail;

	result = setjmp(curthread->t_machdep.tm_copyjmp);
	if (result) {
		curthread->t_machdep.tm_badfaultfunc = NULL;
		return EFAULT;
	}

	result = copystr(dest, (const char *)usersrc, len, stoplen, actual);

	curthread->t_machdep.tm_badfaultfunc = NULL;
	return result;
}

/*
 * copyoutstr
 *
 * Copy a string from kernel address SRC to user-level address
 * USERDEST, as per copystr above. Uses the tm_badfaultfunc/copyfail
 * logic to protect against invalid addresses supplied by a user
 * process.
 */
int
copyoutstr(const char *src, userptr_t userdest, size_t len, size_t *actual)
{
	int result;
	size_t stoplen;

	result = copycheck(userdest, len, &stoplen);
	if (result) {
		return result;
	}

	curthread->t_machdep.tm_badfaultfunc = copyfail;

	result = setjmp(curthread->t_machdep.tm_copyjmp);
	if (result) {
		curthread->t_machdep.tm_badfaultfunc = NULL;
		return EFAULT;
	}

	result = copystr((char *)userdest, src, len, stoplen, actual);

	curthread->t_machdep.tm_badfaultfunc = NULL;
	return result;
}
