/*
 * Copyright (c) 2004, 2008
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

#ifndef _KERN_SOCKET_H_
#define _KERN_SOCKET_H_

/*
 * Socket-related definitions, for <sys/socket.h>.
 */


/*
 * Important
 */

/* Socket types that we (might) support. */
#define SOCK_STREAM	1	/* stream */
#define SOCK_DGRAM	2	/* packet */
#define SOCK_RAW	3	/* raw packet */

/* Address families that we (might) support. */
#define AF_UNSPEC	0
#define AF_UNIX		1
#define AF_INET		2
#define AF_INET6	3

/* Protocol families. Pointless layer of indirection in the standard API. */
#define PF_UNSPEC	AF_UNSPEC
#define PF_UNIX		AF_UNIX
#define PF_INET		AF_INET
#define PF_INET6	AF_INET6

/*
 * Socket address structures. Socket addresses are polymorphic, and
 * the polymorphism is handled by casting pointers. It's fairly gross,
 * but way too deeply standardized to ever change.
 *
 * Each address family defines a sockaddr type (sockaddr_un,
 * sockaddr_in, etc.) struct sockaddr is the common prefix of all
 * these, and struct sockaddr_storage is defined to be large enough to
 * hold any of them.
 *
 * The complex padding in sockaddr_storage forces it to be aligned,
 * which wouldn't happen if it were just a char array.
 */

struct sockaddr {
   __u8	sa_len;
   __u8 sa_family;
};

#define _SS_SIZE	128
struct sockaddr_storage {
   __u8 ss_len;
   __u8 ss_family;
   __u8 __ss_pad1;
   __u8 __ss_pad2;
   __u32 __ss_pad3;
   __u64 __ss_pad4;
   char __ss_pad5[_SS_SIZE - sizeof(__u64) - sizeof(__u32) - 4*sizeof(__u8)];
};


/*
 * Not very important.
 */

/*
 * msghdr structures for sendmsg() and recvmsg().
 */

struct msghdr {
	void *msg_name;		/* really sockaddr; address, or null */
	socklen_t msg_namelen;	/* size of msg_name object, or 0 */
	struct iovec *msg_iov;	/* I/O buffers */
	int msg_iovlen;		/* number of iovecs */
	void *msg_control;	/* auxiliary data area, or null */
	socklen_t msg_controllen; /* size of msg_control area */
	int msg_flags;		/* flags */
};

struct cmsghdr {
	socklen_t cmsg_len;	/* length of control data, including header */
	int cmsg_level;		/* protocol layer item originates from */
	int cmsg_type;		/* protocol-specific message type */
	/* char cmsg_data[];*/	/* data follows the header */
};


#endif /* _KERN_SOCKET_H_ */
