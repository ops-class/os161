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
#include <kern/fcntl.h>
#include <lib.h>
#include <uio.h>
#include <vfs.h>
#include <generic/random.h>
#include "autoconf.h"

/*
 * Machine-independent generic randomness device.
 *
 * Remembers something that's a random source, and provides random()
 * and randmax() to the rest of the kernel.
 *
 * The kernel config mechanism can be used to explicitly choose which
 * of the available random sources to use, if more than one is
 * available.
 */

static struct random_softc *the_random = NULL;

/*
 * VFS device functions.
 * open: allow reading only.
 */
static
int
randeachopen(struct device *dev, int openflags)
{
	(void)dev;

	if (openflags != O_RDONLY) {
		return EIO;
	}

	return 0;
}

/*
 * VFS I/O function. Hand off to implementation.
 */
static
int
randio(struct device *dev, struct uio *uio)
{
	struct random_softc *rs = dev->d_data;

	if (uio->uio_rw != UIO_READ) {
		return EIO;
	}

	return rs->rs_read(rs->rs_devdata, uio);
}

/*
 * VFS ioctl function.
 */
static
int
randioctl(struct device *dev, int op, userptr_t data)
{
	/*
	 * We don't support any ioctls.
	 */
	(void)dev;
	(void)op;
	(void)data;
	return EIOCTL;
}

static const struct device_ops random_devops = {
	.devop_eachopen = randeachopen,
	.devop_io = randio,
	.devop_ioctl = randioctl,
};

/*
 * Config function.
 */
int
config_random(struct random_softc *rs, int unit)
{
	int result;

	/* We use only the first random device. */
	if (unit!=0) {
		return ENODEV;
	}

	KASSERT(the_random==NULL);
	the_random = rs;

	rs->rs_dev.d_ops = &random_devops;
	rs->rs_dev.d_blocks = 0;
	rs->rs_dev.d_blocksize = 1;
	rs->rs_dev.d_data = rs;

	/* Add the VFS device structure to the VFS device list. */
	result = vfs_adddev("random", &rs->rs_dev, 0);
	if (result) {
		return result;
	}

	return 0;
}


/*
 * Random number functions exported to the rest of the kernel.
 */

uint32_t
random(void)
{
	if (the_random==NULL) {
		panic("No random device\n");
	}
	return the_random->rs_random(the_random->rs_devdata);
}

uint32_t
randmax(void)
{
	if (the_random==NULL) {
		panic("No random device\n");
	}
	return the_random->rs_randmax(the_random->rs_devdata);
}
