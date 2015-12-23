# Makefile for badcall

TOP=../../..
.include "$(TOP)/mk/os161.config.mk"

SRCS=\
	bad_execv.c \
	bad_waitpid.c \
	bad_open.c \
	bad_read.c \
	bad_write.c \
	bad_close.c \
	bad_reboot.c \
	bad_sbrk.c \
	bad_ioctl.c \
	bad_lseek.c \
	bad_fsync.c \
	bad_ftruncate.c \
	bad_stat.c \
	bad_remove.c \
	bad_rename.c \
	bad_link.c \
	bad_mkdir.c \
	bad_rmdir.c \
	bad_chdir.c \
	bad_getdirentry.c \
	bad_symlink.c \
	bad_readlink.c \
	bad_dup2.c \
	bad_pipe.c \
	bad_time.c \
	bad_getcwd.c \
	common_buf.c \
	common_fds.c \
	common_path.c \
	report.c \
	driver.c

PROG=badcall
BINDIR=/testbin

.include "$(TOP)/mk/os161.prog.mk"

