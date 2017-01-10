#
# Toplevel makefile for OS/161.
#
#
# Main rules:
#    all (default):  depend and compile system; install into staging area
#    rebuild:        likewise, but start with a clean slate.
#    fullrebuild:    likewise, but start with a very clean slate.
#
# What all does, in order:
#    tools:          depend and compile the tools used in build.
#    includes:       install header files.
#    build:          depend and compile the system.
#
# Other targets:
#    depend:         just update make dependency information.
#    tags:           generate/regenerate "tags" files.
#    install:        install into $(OSTREE).
#    clean:          remove generated files.
#    distclean:      remove all generated files.
#

TOP=.
.include "$(TOP)/mk/os161.config.mk"

all:;  # make this first

MKDIRS=$(OSTREE)

.include "$(TOP)/mk/os161.mkdirs.mk"

all: tools .WAIT includes .WAIT build

rebuild:
	$(MAKE) clean
	$(MAKE) all

fullrebuild:
	$(MAKE) distclean
	$(MAKE) all

# currently no tools required, hence no tools/ dir or work to do
tools:
	@true

build:
	(cd userland && $(MAKE) build)
	(cd man && $(MAKE) install-staging)

includes tags depend:
	(cd kern && $(MAKE) $@)
	(cd userland && $(MAKE) $@)

clean:
	(cd kern && $(MAKE) $@)
	(cd userland && $(MAKE) $@)
	rm -rf $(INSTALLTOP)

distclean: clean
	rm -rf $(WORKDIR)

install: $(OSTREE)
	(cd $(INSTALLTOP) && tar -cf - .) | (cd $(OSTREE) && tar -xvf -)


.PHONY: all rebuild fullrebuild tools build includes tags depend
.PHONY: clean distclean

# old BSD name, same as distclean
cleandir: distclean
.PHONY: cleandir
