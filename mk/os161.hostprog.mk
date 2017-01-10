#
# OS/161 build environment: build a host-system program
#
# Usage:
#    TOP=../..
#    .include "$(TOP)/mk/os161.config.mk"
#    [defs go here]
#    .include "$(TOP)/mk/os161.hostprog.mk"
#    [any extra rules go here]
#
# Variables controlling this file:
#
# PROG				Name of program to generate.
# SRCS				.c and .S files to compile.
#
# HOST_CFLAGS			Compile flags.
# HOST_LDFLAGS			Link flags.
# HOST_LIBS			Libraries to link with.
#
# HOSTBINDIR			Directory under $(OSTREE) to install into,
#                               e.g. /hostbin. Should have a leading slash.
#				If not set, the program goes in
#				$(TOOLDIR)/hostbin instead.
#
# Note that individual program makefiles should only *append* to
# HOST_CFLAGS, HOST_LDFLAGS, and HOST_LIBS, not assign them. Otherwise
# stuff set by os161.config.mk will get lost and bad things will
# happen.
#
# This is set up so it can be used together with os161.prog.mk if
# necessary.
#

# Real name
_PROG_=host-$(PROG)

# Directory to install into
.if defined(HOSTBINDIR)
_INSTALLDIR_=$(INSTALLTOP)$(HOSTBINDIR)
.else
_INSTALLDIR_=$(TOOLDIR)/hostbin
.endif

# We may want these directories created. (Used by os161.baserules.mk.)
MKDIRS+=$(MYBUILDDIR)
MKDIRS+=$(_INSTALLDIR_)
.if defined(HOSTBINDIR)
MKDIRS+=$(OSTREE)$(HOSTBINDIR)
.endif

# Default rule: create the program.
# (In make the first rule found is the default.)
all: all-local
all-local: $(MYBUILDDIR) .WAIT $(MYBUILDDIR)/$(_PROG_)

# Now get rules to compile the SRCS.
.include "$(TOP)/mk/os161.hostcompile.mk"

# Further rules for programs.

# Clean: delete extraneous files.
clean-local: cleanhostprog
cleanhostprog:
	rm -f $(MYBUILDDIR)/$(_PROG_)

#
# Install: we can install into either $(INSTALLTOP) or $(OSTREE).
# When building the whole system, we always install into the staging
# area. However, if you're working on a particular program it is
# usually convenient to be able to install it directly to $(OSTREE)
# instead of doing a complete top-level install.
#
# Note that we make a hard link instead of a copy by default to reduce
# overhead.
#
install-staging-local: $(_INSTALLDIR_) .WAIT $(_INSTALLDIR_)/$(_PROG_)
$(_INSTALLDIR_)/$(_PROG_): $(MYBUILDDIR)/$(_PROG_)
	rm -f $(.TARGET)
	ln $(MYBUILDDIR)/$(_PROG_) $(.TARGET) || \
	  cp $(MYBUILDDIR)/$(_PROG_) $(.TARGET)

.if defined(HOSTBINDIR)
install-local: install-hostprog
install-hostprog: $(OSTREE)$(HOSTBINDIR) $(MYBUILDDIR)/$(_PROG_)
	rm -f $(OSTREE)$(HOSTBINDIR)/$(_PROG_)
	ln $(MYBUILDDIR)/$(_PROG_) $(OSTREE)$(HOSTBINDIR)/$(_PROG_) || \
	  cp $(MYBUILDDIR)/$(_PROG_) $(OSTREE)$(HOSTBINDIR)/$(_PROG_)
.else
install-local:
	@echo "Nothing to manually install"
.endif

# Always implicitly include the compat library.
_COMPATLIB_=$(TOOLDIR)/hostlib/libhostcompat.a

# Link the program.
$(MYBUILDDIR)/$(_PROG_): $(HOST_OBJS) $(_COMPATLIB_)
	$(HOST_LDCC) $(HOST_LDFLAGS) $(HOST_OBJS) $(HOST_LIBS) $(_COMPATLIB_) \
	   -o $(.TARGET)

# Mark targets that don't represent files PHONY, to prevent various
# lossage if files by those names appear.
.PHONY: all all-local clean-local cleanhostprog install-staging-local
.PHONY:  install-local install-hostprog

# Finally, get the shared definitions for the most basic rules.
.include "$(TOP)/mk/os161.baserules.mk"

# End.
