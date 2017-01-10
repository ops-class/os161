#
# OS/161 build environment: install man pages
#
# Usage:
#    TOP=../..
#    .include "$(TOP)/mk/os161.config.mk"
#    [defs go here]
#    .include "$(TOP)/mk/os161.man.mk"
#    [any extra rules go here]
#
# Variables controlling this file:
#
# MANFILES			Files to install.
#
# MANDIR			Directory under $(OSTREE) to install into,
#                               e.g. /man/bin. Should have a leading slash.
#

# We may want these directories created. (Used by os161.baserules.mk.)
# We don't use the build directory because we don't currently generate
# anything for man pages.
#MKDIRS+=$(MYBUILDDIR)
MKDIRS+=$(INSTALLTOP)$(MANDIR)
MKDIRS+=$(OSTREE)$(MANDIR)

# Default rule: do nothing.
# (In make the first rule found is the default.)
all: all-local
all-local: ;

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
install-staging-local: $(INSTALLTOP)$(MANDIR) .WAIT

.for _F_ in $(MANFILES)
install-staging-local: $(INSTALLTOP)$(MANDIR)/$(_F_)
$(INSTALLTOP)$(MANDIR)/$(_F_): $(_F_)
	rm -f $(.TARGET)
	ln $(_F_) $(.TARGET) || cp $(_F_) $(.TARGET)
.endfor

install-local: $(OSTREE)$(MANDIR) .WAIT installmanpages
installmanpages:
.for _F_ in $(MANFILES)
	rm -f $(OSTREE)$(MANDIR)/$(_F_)
	ln $(_F_) $(OSTREE)$(MANDIR)/$(_F_) || \
	  cp $(_F_) $(OSTREE)$(MANDIR)/$(_F_)
.endfor

# clean: remove build products (nothing to do)
clean-local: ;

# Mark targets that don't represent files PHONY, to prevent various
# lossage if files by those names appear.
.PHONY: all all-local install-staging-local install-local installmanpages
.PHONY: clean-local

# Finally, get the shared definitions for the most basic rules.
.include "$(TOP)/mk/os161.baserules.mk"

# End.
