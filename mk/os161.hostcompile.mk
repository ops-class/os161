#
# OS/161 build environment: compile source files for the host system.
#
# Usage: use os161.hostprog.mk or os161.hostlib.mk
#
# Variables controlling this file:
#
# SRCS				.c and .S files to compile.
#
# Provides:
#
# HOST_OBJS			.ho files from compilation.
#

# Objects list starts empty. It is added to below.
HOST_OBJS=

# .ho is a host object.
.SUFFIXES: .ho

clean-local: cleanhostcompile
cleanhostcompile:
	rm -f $(MYBUILDDIR)/*.ho $(MYBUILDDIR)/*.ha

distclean-local: distcleanhostcompile
distcleanhostcompile:
	rm -f $(MYBUILDDIR)/.hostdepend

#
# Depend: generate dependency information.
# Use gcc's -MM argument for this.
#
# Note that we use -MM rather than -M, so we don't get system headers.
# They would be host system headers and we don't want to get involved
# with those.
#
# The fixdepends script transforms the results by substituting some
# make variables back into them; this way the depend files are
# independent of (at least some of) the build configuration. It also
# changes .o files to .ho files so the host and native builds are
# independent, and allows for placing the .ho files in the build
# directory.
#
depend-local: $(MYBUILDDIR) .WAIT predepend .WAIT dependhostcompile
dependhostcompile:
	$(HOST_CC) $(HOST_CFLAGS) -DHOST -MM $(SRCS) |\
		$(TOP)/mk/fixdepends.sh '$(INSTALLTOP)' host \
		> $(MYBUILDDIR)/.hostdeptmp
	mv -f $(MYBUILDDIR)/.hostdeptmp $(MYBUILDDIR)/.hostdepend

.-include "$(MYBUILDDIR)/.hostdepend"

predepend:
.PHONY: predepend

# No tags for host programs.
tags: tagshostcompile
tagshostcompile: ;

#
# Compile rules.
# We can use the same rules for .c and .S because gcc knows how to handle
# .S files.
#
.for _S_ in $(SRCS:M*.[cS])
HOST_OBJS+=$(MYBUILDDIR)/$(_S_:T:R).ho
$(MYBUILDDIR)/$(_S_:T:R).ho: $(_S_)
	$(HOST_CC) $(HOST_CFLAGS) -DHOST -c $(_S_) -o $(.TARGET)
.endfor

# Make non-file rules PHONY.
.PHONY: clean-local cleanhostcompile distclean-local distcleanhostcompile
.PHONY: depend-local dependhostcompile tags tagshostcompile

# End.
