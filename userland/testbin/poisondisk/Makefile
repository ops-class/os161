# Makefile for poisondisk

TOP=../../..
.include "$(TOP)/mk/os161.config.mk"

PROG=poisondisk
SRCS=poisondisk.c \
	../../sbin/mksfs/disk.c ../../sbin/mksfs/support.c
CFLAGS+=-I../../sbin/mksfs
HOST_CFLAGS+=-I../../sbin/mksfs
BINDIR=/testbin
HOSTBINDIR=/hostbin


.include "$(TOP)/mk/os161.prog.mk"
.include "$(TOP)/mk/os161.hostprog.mk"
