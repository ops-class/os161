#
# OS/161 build environment: install scripts
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
# EXECSCRIPTS			Executable files to install.
# NONEXECSCRIPTS		Non-executable files to install.
#
# SCRIPTDIR			Directory under $(OSTREE) to install into,
#                               e.g. /hostbin. Should have a leading slash.
#

ALLSCRIPTS=$(NONEXECSCRIPTS)

# We may want these directories created. (Used by os161.baserules.mk.)
MKDIRS+=$(MYBUILDDIR)
MKDIRS+=$(INSTALLTOP)$(SCRIPTDIR)
MKDIRS+=$(OSTREE)$(SCRIPTDIR)

# Default rule: "build" the executable scripts. This sets the hash-bang
# header with the interpreter path.
# (In make the first rule found is the default.)
all: all-local
all-local: $(MYBUILDDIR) .WAIT
.for S in $(EXECSCRIPTS)
ALLSCRIPTS+=$(MYBUILDDIR)/$(S)
all-local: $(MYBUILDDIR)/$(S)
$(MYBUILDDIR)/$(S): $(S)
# This must test $@, or assign another variable; it can't test $(S).
# (This is a bmake bug.)
.if !empty(@:M*.py)
	echo '#!'"$(PYTHON_INTERPRETER)" > $@.new
.else
	sed -n -e '1p' < $(S) > $@.new
.endif
	sed -e '1d' < $(S) >> $@.new
	chmod 755 $@.new
	mv -f $@.new $@
.endfor

# depend doesn't need to do anything
depend-local: ;

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
install-staging-local: $(INSTALLTOP)$(SCRIPTDIR) .WAIT

.for _F_ in $(ALLSCRIPTS)
install-staging-local: $(INSTALLTOP)$(SCRIPTDIR)/$(_F_:T)
$(INSTALLTOP)$(SCRIPTDIR)/$(_F_:T): $(_F_)
	rm -f $(.TARGET)
	ln $(_F_) $(.TARGET) || cp $(_F_) $(.TARGET)
.endfor

install-local: $(OSTREE)$(SCRIPTDIR) .WAIT installscripts
installscripts:
.for _F_ in $(ALLSCRIPTS)
	rm -f $(OSTREE)$(SCRIPTDIR)/$(_F_:T)
	ln $(_F_) $(OSTREE)$(SCRIPTDIR)/$(_F_:T) || \
	  cp $(_F_) $(OSTREE)$(SCRIPTDIR)/$(_F_:T)
.endfor

# clean: remove build products
clean-local:
.for S in $(EXECSCRIPTS)
	rm -f $(MYBUILDDIR)/$(S)
.endfor

# Mark targets that don't represent files PHONY, to prevent various
# lossage if files by those names appear.
.PHONY: all all-local install-staging-local install-local installscripts
.PHONY: clean-local

# Finally, get the shared definitions for the most basic rules.
.include "$(TOP)/mk/os161.baserules.mk"

# End.
