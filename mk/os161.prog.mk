#
# OS/161 build environment: build a program
#
# Usage:
#    TOP=../..
#    .include "$(TOP)/mk/os161.config.mk"
#    [defs go here]
#    .include "$(TOP)/mk/os161.prog.mk"
#    [any extra rules go here]
#
# Variables controlling this file:
#
# PROG				Name of program to generate.
# SRCS				.c and .S files to compile.
#
# CFLAGS			Compile flags.
# LDFLAGS			Link flags.
# LIBS				Libraries to link with.
# LIBDEPS			Full paths of LIBS for depending on.
#
# BINDIR			Directory under $(OSTREE) to install into,
#                               e.g. /bin. Should have a leading slash.
#
# Note that individual program makefiles should only *append* to
# CFLAGS, LDFLAGS, LIBS, and LIBDEPS, not assign them. Otherwise stuff
# set by os161.config.mk will get lost and bad things will happen.
#

BINDIR?=/bin

# We may want these directories created. (Used by os161.baserules.mk.)
MKDIRS+=$(MYBUILDDIR)
MKDIRS+=$(INSTALLTOP)$(BINDIR)
MKDIRS+=$(OSTREE)$(BINDIR)

# Default rule: create the program.
# (In make the first rule found is the default.)
all: all-local
all-local: $(MYBUILDDIR) .WAIT $(MYBUILDDIR)/$(PROG)

# Now get rules to compile the SRCS.
.include "$(TOP)/mk/os161.compile.mk"

# Further rules for programs.

# Clean: delete extraneous files.
clean-local: cleanprog
cleanprog:
	rm -f $(MYBUILDDIR)/$(PROG)

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
install-staging-local: $(INSTALLTOP)$(BINDIR) .WAIT $(INSTALLTOP)$(BINDIR)/$(PROG)
$(INSTALLTOP)$(BINDIR)/$(PROG): $(MYBUILDDIR)/$(PROG)
	rm -f $(.TARGET)
	ln $(MYBUILDDIR)/$(PROG) $(.TARGET) || \
	  cp $(MYBUILDDIR)/$(PROG) $(.TARGET)

install-local: install-prog
install-prog: $(OSTREE)$(BINDIR) $(MYBUILDDIR)/$(PROG)
	rm -f $(OSTREE)$(BINDIR)/$(PROG)
	ln $(MYBUILDDIR)/$(PROG) $(OSTREE)$(BINDIR)/$(PROG) || \
	  cp $(MYBUILDDIR)/$(PROG) $(OSTREE)$(BINDIR)/$(PROG)

# Link the program.
$(MYBUILDDIR)/$(PROG): $(OBJS) $(LIBDEPS)
	$(LDCC) $(LDFLAGS) $(OBJS) $(LIBS) $(MORELIBS) -o $(.TARGET)

# Mark targets that don't represent files PHONY, to prevent various
# lossage if files by those names appear.
.PHONY: all all-local clean cleanprog install-staging-local
.PHONY: install-local install-prog

# Finally, get the shared definitions for the most basic rules.
.include "$(TOP)/mk/os161.baserules.mk"

# End.
