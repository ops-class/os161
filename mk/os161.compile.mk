#
# OS/161 build environment: compile source files.
#
# Usage: use os161.prog.mk or os161.lib.mk
#
# Variables controlling this file:
#
# SRCS				.c and .S files to compile.
#
# Provides:
#
# OBJS				.o files from compilation.
#

# Objects list starts empty. It is added to below.
OBJS=

clean-local: cleancompile
cleancompile:
	rm -f $(MYBUILDDIR)/*.[oa]

distclean-local: distcleancompile
distcleancompile:
	rm -f $(MYBUILDDIR)/.depend

#
# Depend: generate dependency information.
# Use gcc's -M argument for this.
#
# Note that we use -M rather than -MM, to get both system headers
# and program-local headers. This is because we *are* the system and
# we might well change those system headers.
#
# The fixdepends script transforms the results by substituting some
# make variables back into them; this way the depend files are
# independent of (at least some of) the build configuration. It also
# allows for placing the .o files in the build directory.
#
depend-local: $(MYBUILDDIR) .WAIT predepend .WAIT dependcompile
dependcompile:
	$(CC) $(CFLAGS) $(MORECFLAGS) -M $(SRCS) |\
		$(TOP)/mk/fixdepends.sh '$(INSTALLTOP)' native \
		> $(MYBUILDDIR)/.deptmp
	mv -f $(MYBUILDDIR)/.deptmp $(MYBUILDDIR)/.depend

.-include "$(MYBUILDDIR)/.depend"

predepend:
.PHONY: predepend

tags: tagscompile
tagscompile:
	ctags -wtd $(SRCS) *.h

#
# Compile rules.
# We can use the same rules for .c and .S because gcc knows how to handle
# .S files.
#
# Make it so typing "make foo.o" does the right thing.
#
.for _S_ in $(SRCS:M*.[cS])
OBJS+=$(MYBUILDDIR)/$(_S_:T:R).o
$(MYBUILDDIR)/$(_S_:T:R).o: $(_S_)
	$(CC) $(CFLAGS) $(MORECFLAGS) -c $(_S_) -o $(.TARGET)

$(_S_:T:R).o: $(MYBUILDDIR)/$(_S_:T:R).o
.PHONY: $(_S_:T:R).o
.endfor

# Make non-file rules PHONY.
.PHONY: clean-local cleancompile distclean-local distcleancompile
.PHONY: depend-local dependcompile tags tagscompile

# End.
