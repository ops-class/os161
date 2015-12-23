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

/*
 * Implementation of the null device, "null:", which generates an
 * immediate EOF on read and throws away anything written to it.
 */
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <uio.h>
#include <vfs.h>
#include <device.h>

/* For open() */
static
int
nullopen(struct device *dev, int openflags)
{
	(void)dev;
	(void)openflags;

	return 0;
}

/* For d_io() */
static
int
nullio(struct device *dev, struct uio *uio)
{
	/*
	 * On write, discard everything without looking at it.
	 * (Notice that you can write to the null device from invalid
	 * buffer pointers and it will still succeed. This behavior is
	 * traditional.)
	 *
	 * On read, do nothing, generating an immediate EOF.
	 */

	(void)dev; // unused

	if (uio->uio_rw == UIO_WRITE) {
		uio->uio_resid = 0;
	}

	return 0;
}

/* For ioctl() */
static
int
nullioctl(struct device *dev, int op, userptr_t data)
{
	/*
	 * No ioctls.
	 */

	(void)dev;
	(void)op;
	(void)data;

	return EINVAL;
}

static const struct device_ops null_devops = {
	.devop_eachopen = nullopen,
	.devop_io = nullio,
	.devop_ioctl = nullioctl,
};

/*
 * Function to create and attach null:
 */
void
devnull_create(void)
{
	int result;
	struct device *dev;

	dev = kmalloc(sizeof(*dev));
	if (dev==NULL) {
		panic("Could not add null device: out of memory\n");
	}

	dev->d_ops = &null_devops;

	dev->d_blocks = 0;
	dev->d_blocksize = 1;

	dev->d_devnumber = 0; /* assigned by vfs_adddev */

	dev->d_data = NULL;

	result = vfs_adddev("null", dev, 0);
	if (result) {
		panic("Could not add null device: %s\n", strerror(result));
	}
}
