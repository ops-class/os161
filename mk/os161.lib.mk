#
# OS/161 build environment: build a library
#
# Usage:
#    TOP=../..
#    .include "$(TOP)/mk/os161.config.mk"
#    [defs go here]
#    .include "$(TOP)/mk/os161.lib.mk"
#    [any extra rules go here]
#
# Variables controlling this file:
#
# LIB				Name of library. We create lib$(LIB).a.
# SRCS				.c and .S files to compile.
#
# CFLAGS			Compile flags.
#
# LIBDIR			Directory under $(OSTREE) to install into,
#                               e.g. /lib. Should have a leading slash.
#
# Note that individual program makefiles should only *append* to
# CFLAGS, not assign it. Otherwise stuff set by os161.config.mk will
# get lost and bad things will happen.
#
# Because we only build static libs, we can't use and don't need
# LDFLAGS, LIBS, or LIBDEPS. (Shared libs would want these.)
#

LIBDIR?=/lib

_LIB_=lib$(LIB).a

# We may want these directories created. (Used by os161.baserules.mk.)
MKDIRS+=$(MYBUILDDIR)
MKDIRS+=$(INSTALLTOP)$(LIBDIR)
MKDIRS+=$(OSTREE)$(LIBDIR)

# Default rule: create the program.
# (In make the first rule found is the default.)
all: all-local
all-local: $(MYBUILDDIR) .WAIT $(MYBUILDDIR)/$(_LIB_)

# Now get rules to compile the SRCS.
.include "$(TOP)/mk/os161.compile.mk"

# Further rules for libraries.

#
# Install: we can install into either $(INSTALLTOP) or $(OSTREE).
# When building the whole system, we always install into the staging
# area. We provide the same direct install that os161.prog.mk
# provides; however, because it this doesn't relink anything using the
# library it generally isn't a very useful thing to do. Hence the
# warning.
#
# Note that we make a hard link instead of a copy by default to reduce
# overhead.
#
install-staging-local: $(INSTALLTOP)$(LIBDIR) .WAIT $(INSTALLTOP)$(LIBDIR)/$(_LIB_)
$(INSTALLTOP)$(LIBDIR)/$(_LIB_): $(MYBUILDDIR)/$(_LIB_)
	rm -f $(.TARGET)
	ln $(MYBUILDDIR)/$(_LIB_) $(.TARGET) >/dev/null 2>&1 || \
	  cp $(MYBUILDDIR)/$(_LIB_) $(.TARGET)

install-local: $(OSTREE)$(LIBDIR) $(MYBUILDDIR)/$(_LIB_)
	@echo "Warning: manually installing library without relinking anything"
	rm -f $(OSTREE)$(LIBDIR)/$(_LIB_)
	ln $(MYBUILDDIR)/$(_LIB_) $(OSTREE)$(LIBDIR)/$(_LIB_) >/dev/null 2>&1 || \
	  cp $(MYBUILDDIR)/$(_LIB_) $(OSTREE)$(LIBDIR)/$(_LIB_)

# Build the library.
$(MYBUILDDIR)/$(_LIB_): $(OBJS)
	rm -f $(.TARGET)
	$(AR) -cq $(.TARGET) $(OBJS)
	$(RANLIB) $(.TARGET)

# Mark targets that don't represent files PHONY, to prevent various
# lossage if files by those names appear.
.PHONY: all all-local install-staging-local install-local

# Finally, get the shared definitions for the most basic rules.
.include "$(TOP)/mk/os161.baserules.mk"

# End.
