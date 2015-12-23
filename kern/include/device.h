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

#ifndef _DEVICE_H_
#define _DEVICE_H_

/*
 * Devices.
 */


struct uio;  /* in <uio.h> */

/*
 * Filesystem-namespace-accessible device.
 */
struct device {
	const struct device_ops *d_ops;

	blkcnt_t d_blocks;
	blksize_t d_blocksize;

	dev_t d_devnumber;	/* serial number for this device */

	void *d_data;		/* device-specific data */
};

/*
 * Device operations.
 *      devop_eachopen - called on each open call to allow denying the open
 *      devop_io - for both reads and writes (the uio indicates the direction)
 *      devop_ioctl - miscellaneous control operations
 */
struct device_ops {
	int (*devop_eachopen)(struct device *, int flags_from_open);
	int (*devop_io)(struct device *, struct uio *);
	int (*devop_ioctl)(struct device *, int op, userptr_t data);
};

/*
 * Macros to shorten the calling sequences.
 */
#define DEVOP_EACHOPEN(d, f)	((d)->d_ops->devop_eachopen(d, f))
#define DEVOP_IO(d, u)		((d)->d_ops->devop_io(d, u))
#define DEVOP_IOCTL(d, op, p)	((d)->d_ops->devop_ioctl(d, op, p))


/* Create vnode for a vfs-level device. */
struct vnode *dev_create_vnode(struct device *dev);

/* Undo dev_create_vnode. */
void dev_uncreate_vnode(struct vnode *vn);

/* Initialization functions for builtin vfs-level devices. */
void devnull_create(void);

/* Function that kicks off device probe and attach. */
void dev_bootstrap(void);


#endif /* _DEVICE_H_ */
