#
# Makefile for OS/161 C standard library
#

TOP=../../..
.include "$(TOP)/mk/os161.config.mk"

COMMON=$(TOP)/common/libc

# printf
SRCS+=\
	$(COMMON)/printf/__printf.c \
	$(COMMON)/printf/snprintf.c \
	$(COMMON)/printf/tprintf.c

# stdio
SRCS+=\
	stdio/__puts.c \
	stdio/getchar.c \
	stdio/printf.c \
	stdio/putchar.c \
	stdio/puts.c

# stdlib
SRCS+=\
	stdlib/abort.c \
	$(COMMON)/stdlib/atoi.c \
	stdlib/exit.c \
	stdlib/getenv.c \
	stdlib/malloc.c \
	stdlib/qsort.c \
	stdlib/random.c \
	stdlib/system.c

# string
SRCS+=\
	$(COMMON)/string/bzero.c \
	string/memcmp.c \
	$(COMMON)/string/memcpy.c \
	$(COMMON)/string/memmove.c \
	$(COMMON)/string/memset.c \
	$(COMMON)/string/strcat.c \
	$(COMMON)/string/strchr.c \
	$(COMMON)/string/strcmp.c \
	$(COMMON)/string/strcpy.c \
	string/strerror.c \
	$(COMMON)/string/strlen.c \
	$(COMMON)/string/strrchr.c \
	string/strtok.c \
	$(COMMON)/string/strtok_r.c

# time
SRCS+=\
	time/time.c

# system call stubs
SRCS+=\
	$(MYBUILDDIR)/syscalls.S

# gcc support
COMMONGCC=$(TOP)/common/gcc-millicode
SRCS+=\
	$(COMMONGCC)/adddi3.c \
	$(COMMONGCC)/anddi3.c \
	$(COMMONGCC)/ashldi3.c \
	$(COMMONGCC)/ashrdi3.c \
	$(COMMONGCC)/cmpdi2.c \
	$(COMMONGCC)/divdi3.c \
	$(COMMONGCC)/iordi3.c \
	$(COMMONGCC)/lshldi3.c \
	$(COMMONGCC)/lshrdi3.c \
	$(COMMONGCC)/moddi3.c \
	$(COMMONGCC)/muldi3.c \
	$(COMMONGCC)/negdi2.c \
	$(COMMONGCC)/notdi2.c \
	$(COMMONGCC)/qdivrem.c \
	$(COMMONGCC)/subdi3.c \
	$(COMMONGCC)/ucmpdi2.c \
	$(COMMONGCC)/udivdi3.c \
	$(COMMONGCC)/umoddi3.c \
	$(COMMONGCC)/xordi3.c


# other stuff
SRCS+=\
	unix/__assert.c \
	unix/err.c \
	unix/errno.c \
	unix/execvp.c \
	unix/getcwd.c \
	$(COMMON)/arch/mips/setjmp.S

# Name of the library.
LIB=c

# Let the templates do most of the work.
.include  "$(TOP)/mk/os161.lib.mk"

#
# Generate syscall entry points from system call list.
#
# Note that this will bomb if the kernel headers haven't been
# installed into the staging area.
#

SYSCALL_H=$(INSTALLTOP)/include/kern/syscall.h

# This is not ideal as it won't rebuild syscalls.S if defs.mk is removed.
# But it's better than failing if defs.mk is not present.
.if exists($(TOP)/defs.mk)
$(MYBUILDDIR)/syscalls.S: $(TOP)/defs.mk
.endif
$(MYBUILDDIR)/syscalls.S: $(SYSCALL_H)
$(MYBUILDDIR)/syscalls.S: syscalls/gensyscalls.sh
$(MYBUILDDIR)/syscalls.S: arch/$(MACHINE)/syscalls-$(MACHINE).S
	-rm -f $@ $@.tmp
	echo '/* Automatically generated; do not edit */' > $@.tmp
	cat arch/$(MACHINE)/syscalls-$(MACHINE).S >> $@.tmp
	syscalls/gensyscalls.sh < $(SYSCALL_H) >> $@.tmp
	mv -f $@.tmp $@

clean: cleanhere
cleanhere:
	rm -f $(MYBUILDDIR)/syscalls.S

predepend:
	$(MAKE) $(MYBUILDDIR)/syscalls.S

.PHONY: clean cleanhere depend predepend

# Have the machine-dependent stuff depend on defs.mk in case MACHINE
# or PLATFORM changes.
setjmp.o: $(TOP)/defs.mk
