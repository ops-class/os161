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

#ifndef _UIO_H_
#define _UIO_H_

/*
 * A uio is an abstraction encapsulating a memory block, some metadata
 * about it, and also a cursor position associated with working
 * through it. The uio structure is used to manage blocks of data
 * moved around by the kernel.
 *
 * Note: struct iovec is in <kern/iovec.h>.
 *
 * The structure here is essentially the same as BSD uio. The
 * position is maintained by incrementing the block pointer,
 * decrementing the block size, decrementing the residue count, and
 * also incrementing the seek offset in uio_offset. The last is
 * intended to provide management for seek pointers.
 *
 * Callers of file system operations that take uios should honor the
 * uio_offset values returned by these operations, as for directories
 * they may not necessarily be byte counts and attempting to compute
 * seek positions based on byte counts can produce wrong behavior.
 *
 * File system operations calling uiomove for directory data and not
 * intending to use byte counts should update uio_offset to the
 * desired value explicitly after calling uiomove, as uiomove always
 * increments uio_offset by the number of bytes transferred.
 */

#include <kern/iovec.h>

/* Direction. */
enum uio_rw {
        UIO_READ,			/* From kernel to uio_seg */
        UIO_WRITE,			/* From uio_seg to kernel */
};

/* Source/destination. */
enum uio_seg {
        UIO_USERISPACE,			/* User process code. */
        UIO_USERSPACE,			/* User process data. */
        UIO_SYSSPACE,			/* Kernel. */
};

struct uio {
	struct iovec     *uio_iov;	/* Data blocks */
	unsigned          uio_iovcnt;	/* Number of iovecs */
	off_t             uio_offset;	/* Desired offset into object */
	size_t            uio_resid;	/* Remaining amt of data to xfer */
	enum uio_seg      uio_segflg;	/* What kind of pointer we have */
	enum uio_rw       uio_rw;	/* Whether op is a read or write */
	struct addrspace *uio_space;	/* Address space for user pointer */
};


/*
 * Copy data from a kernel buffer to a data region defined by a uio struct,
 * updating the uio struct's offset and resid fields. May alter the iovec
 * fields as well.
 *
 * Before calling this, you should
 *   (1) set up uio_iov to point to the buffer(s) you want to transfer
 *       to, and set uio_iovcnt to the number of such buffers;
 *   (2) initialize uio_offset as desired;
 *   (3) initialize uio_resid to the total amount of data that can be
 *       transferred through this uio;
 *   (4) set up uio_seg and uio_rw correctly;
 *   (5) if uio_seg is UIO_SYSSPACE, set uio_space to NULL; otherwise,
 *       initialize uio_space to the address space in which the buffer
 *       should be found.
 *
 * After calling,
 *   (1) the contents of uio_iov and uio_iovcnt may be altered and
 *       should not be interpreted;
 *   (2) uio_offset will have been incremented by the amount transferred;
 *   (3) uio_resid will have been decremented by the amount transferred;
 *   (4) uio_segflg, uio_rw, and uio_space will be unchanged.
 *
 * uiomove() may be called repeatedly on the same uio to transfer
 * additional data until the available buffer space the uio refers to
 * is exhausted.
 *
 * Note that the actual value of uio_offset is not interpreted. It is
 * provided (and updated by uiomove) to allow for easier file seek
 * pointer management.
 *
 * When uiomove is called, the address space presently in context must
 * be the same as the one recorded in uio_space. This is an important
 * sanity check if I/O has been queued.
 */
int uiomove(void *kbuffer, size_t len, struct uio *uio);

/*
 * Like uiomove, but sends zeros.
 */
int uiomovezeros(size_t len, struct uio *uio);

/*
 * Initialize a uio suitable for I/O from a kernel buffer.
 *
 * Usage example;
 * 	char buf[128];
 * 	struct iovec iov;
 * 	struct uio myuio;
 *
 * 	uio_kinit(&iov, &myuio, buf, sizeof(buf), 0, UIO_READ);
 *      result = VOP_READ(vn, &myuio);
 *      ...
 */
void uio_kinit(struct iovec *, struct uio *,
	       void *kbuf, size_t len, off_t pos, enum uio_rw rw);


#endif /* _UIO_H_ */
