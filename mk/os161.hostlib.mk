#
# OS/161 build environment: build a library for the compile host
#
# Usage:
#    TOP=../..
#    .include "$(TOP)/mk/os161.config.mk"
#    [defs go here]
#    .include "$(TOP)/mk/os161.hostlib.mk"
#    [any extra rules go here]
#
# Variables controlling this file:
#
# LIB				Name of library. We create lib$(LIB).a.
# SRCS				.c and .S files to compile.
#
# HOST_CFLAGS			Compile flags.
#
# Note that individual program makefiles should only *append* to
# HOST_CFLAGS, not assign it. Otherwise stuff set by os161.config.mk
# will get lost and bad things will happen.
#
# Because we only build static libs, we can't use and don't need
# LDFLAGS or LIBS. (Shared libs would want these.)
#
# Note that there's no HOSTLIBDIR; host libs are always put in
# $(TOOLDIR)/hostlib and do not end up in $(OSTREE).
#

_LIB_=lib$(LIB).a

# We may want these directories created. (Used by os161.baserules.mk.)
MKDIRS+=$(MYBUILDDIR)
MKDIRS+=$(TOOLDIR)/hostlib

# Default rule: create the program.
# (In make the first rule found is the default.)
all: all-local
all-local: $(MYBUILDDIR) .WAIT $(MYBUILDDIR)/$(_LIB_)

# Now get rules to compile the SRCS.
.include "$(TOP)/mk/os161.hostcompile.mk"

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
install-staging-local: $(TOOLDIR)/hostlib .WAIT $(TOOLDIR)/hostlib/$(_LIB_)
$(TOOLDIR)/hostlib/$(_LIB_): $(MYBUILDDIR)/$(_LIB_)
	rm -f $(.TARGET)
	ln $(MYBUILDDIR)/$(_LIB_) $(.TARGET) >/dev/null 2>&1 || \
	  cp $(MYBUILDDIR)/$(_LIB_) $(.TARGET)

install-local:
	@echo "Nothing to manually install"

# Build the library.
$(MYBUILDDIR)/$(_LIB_): $(HOST_OBJS)
	rm -f $(.TARGET)
	$(HOST_AR) -cq $(.TARGET) $(HOST_OBJS)
	$(HOST_RANLIB) $(.TARGET)

# Mark targets that don't represent files PHONY, to prevent various
# lossage if files by those names appear.
.PHONY: all all-local install-staging-local install-local

# Finally, get the shared definitions for the most basic rules.
.include "$(TOP)/mk/os161.baserules.mk"

# End.
