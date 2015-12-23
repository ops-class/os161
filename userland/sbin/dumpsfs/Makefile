# Makefile for dumpsfs

TOP=../../..
.include "$(TOP)/mk/os161.config.mk"

PROG=dumpsfs
SRCS=dumpsfs.c ../mksfs/disk.c ../mksfs/support.c
CFLAGS+=-I../mksfs
HOST_CFLAGS+=-I../mksfs
BINDIR=/sbin
HOSTBINDIR=/hostbin


.include "$(TOP)/mk/os161.prog.mk"
.include "$(TOP)/mk/os161.hostprog.mk"
