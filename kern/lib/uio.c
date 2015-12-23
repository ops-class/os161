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
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <copyinout.h>

/*
 * See uio.h for a description.
 */

int
uiomove(void *ptr, size_t n, struct uio *uio)
{
	struct iovec *iov;
	size_t size;
	int result;

	if (uio->uio_rw != UIO_READ && uio->uio_rw != UIO_WRITE) {
		panic("uiomove: Invalid uio_rw %d\n", (int) uio->uio_rw);
	}
	if (uio->uio_segflg==UIO_SYSSPACE) {
		KASSERT(uio->uio_space == NULL);
	}
	else {
		KASSERT(uio->uio_space == proc_getas());
	}

	while (n > 0 && uio->uio_resid > 0) {
		/* get the first iovec */
		iov = uio->uio_iov;
		size = iov->iov_len;

		if (size > n) {
			size = n;
		}

		if (size == 0) {
			/* move to the next iovec and try again */
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt == 0) {
				/*
				 * This should only happen if you set
				 * uio_resid incorrectly (to more than
				 * the total length of buffers the uio
				 * points to).
				 */
				panic("uiomove: ran out of buffers\n");
			}
			continue;
		}

		switch (uio->uio_segflg) {
		    case UIO_SYSSPACE:
			    if (uio->uio_rw == UIO_READ) {
				    memmove(iov->iov_kbase, ptr, size);
			    }
			    else {
				    memmove(ptr, iov->iov_kbase, size);
			    }
			    iov->iov_kbase = ((char *)iov->iov_kbase+size);
			    break;
		    case UIO_USERSPACE:
		    case UIO_USERISPACE:
			    if (uio->uio_rw == UIO_READ) {
				    result = copyout(ptr, iov->iov_ubase,size);
			    }
			    else {
				    result = copyin(iov->iov_ubase, ptr, size);
			    }
			    if (result) {
				    return result;
			    }
			    iov->iov_ubase += size;
			    break;
		    default:
			    panic("uiomove: Invalid uio_segflg %d\n",
				  (int)uio->uio_segflg);
		}

		iov->iov_len -= size;
		uio->uio_resid -= size;
		uio->uio_offset += size;
		ptr = ((char *)ptr + size);
		n -= size;
	}

	return 0;
}

int
uiomovezeros(size_t n, struct uio *uio)
{
	/* static, so initialized as zero */
	static char zeros[16];
	size_t amt;
	int result;

	/* This only makes sense when reading */
	KASSERT(uio->uio_rw == UIO_READ);

	while (n > 0) {
		amt = sizeof(zeros);
		if (amt > n) {
			amt = n;
		}
		result = uiomove(zeros, amt, uio);
		if (result) {
			return result;
		}
		n -= amt;
	}

	return 0;
}

/*
 * Convenience function to initialize an iovec and uio for kernel I/O.
 */

void
uio_kinit(struct iovec *iov, struct uio *u,
	  void *kbuf, size_t len, off_t pos, enum uio_rw rw)
{
	iov->iov_kbase = kbuf;
	iov->iov_len = len;
	u->uio_iov = iov;
	u->uio_iovcnt = 1;
	u->uio_offset = pos;
	u->uio_resid = len;
	u->uio_segflg = UIO_SYSSPACE;
	u->uio_rw = rw;
	u->uio_space = NULL;
}
