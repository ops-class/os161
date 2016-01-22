#
# Makefile for hostcompat library
#
# defs.mk contains two special settings for us.
#
#   COMPAT_CFLAGS  contains our configuration cflags.
#   COMPAT_TARGETS are additional targets to run at install time.
#
# COMPAT_CFLAGS may include any of the following:
#
#   -DNEED_ERR     Compile err, errx, etc.
#   -DNEED_NTOHLL  Compile ntohll and htonll
#
# COMPAT_TARGETS may include any of the following:
#
#   install-errh   Install an <err.h>
#

TOP=../../..
.include "$(TOP)/mk/os161.config.mk"

LIB=hostcompat
SRCS=err.c ntohll.c time.c hostcompat.c

# printf
COMMON=$(TOP)/common/libc
SRCS+=\
	$(COMMON)/printf/tprintf.c

HOST_CFLAGS+=$(COMPAT_CFLAGS)

MKDIRS=$(INSTALLTOP)/hostinclude

.include "$(TOP)/mk/os161.hostlib.mk"

# XXX: installheaders.sh should be made flexible enough to be used here
includes: $(INSTALLTOP)/hostinclude .WAIT $(COMPAT_TARGETS)
	@if diff hostcompat.h \
		$(INSTALLTOP)/hostinclude/hostcompat.h > /dev/null 2>&1; then \
	     :;								\
	 else								\
	     echo cp hostcompat.h $(INSTALLTOP)/hostinclude/;		\
	     cp hostcompat.h $(INSTALLTOP)/hostinclude/;		\
	 fi
	[ -h $(INSTALLTOP)/hostinclude/kern ] || \
		ln -sf ../include/kern $(INSTALLTOP)/hostinclude/kern

install-errh:
	@if diff host-err.h \
		$(INSTALLTOP)/hostinclude/err.h > /dev/null 2>&1; then	\
	     :;								\
	 else								\
	     echo cp host-err.h $(INSTALLTOP)/hostinclude/err.h;	\
	     cp host-err.h $(INSTALLTOP)/hostinclude/err.h;		\
	 fi

# Recompile these if the config changes, as they might change with it
err.o ntohll.o: $(TOP)/defs.mk

.PHONY: includes install-errh
