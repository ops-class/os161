#
# OS/161 build environment: some very basic rules.
#
# Individual program makefiles should use os161.prog.mk or
# os161.lib.mk instead of including this file directly.
#
# The variable MKDIRS is used to generate rules for creating
# (mostly installation) directories via os161.mkdirs.mk.

# Process this file only once even if included repeatedly
.if !defined(_BASERULES_MK_)
_BASERULES_MK_=# empty


#
# Establish that all these (basic) rules exist and depend on the
# local (non-subdir) version.
#
.for _T_ in all depend install install-staging clean distclean tags
$(_T_): ;
$(_T_): $(_T_)-local
.PHONY: $(_T_) $(_T_)-local
.endfor

# distclean implies clean
distclean: clean-local


#
# Some other derived rules.
#

# cleandir is the same as distclean (cleandir is the old BSD name)
cleandir: distclean-local

# "stage" is a good short name for install-staging
stage: install-staging-local

# dependall means depend then compile, but it's important to run a
# new make after depending so the new depends take effect.
dependall-local: depend-local
	$(MAKE) all-local
dependall: dependall-local

# build means depend, compile, and install-staging; it also needs a
# new make after depending. It could use the same one for compile
# and install-staging, but that turns out to be awkward.
build-local: dependall-local
	$(MAKE) install-staging-local
build: build-local

# rebuild cleans first
rebuild: clean-local .WAIT build-local

# fullrebuild does distclean
fullrebuild: distclean-local .WAIT build-local

# implement BUILDSYMLINKS
.if "$(BUILDSYMLINKS)" == "yes"
.if !exists(build)
all depend: buildlink
.endif
buildlink:
	ln -s $(MYBUILDDIR) build
clean: remove-buildlink
remove-buildlink:
	rm -f build
.PHONY: buildlink remove-buildlink
.endif

.PHONY: cleandir stage dependall build rebuild fullrebuild
.PHONY: dependall-local build-local

.endif # _BASERULES_MK_

.include "$(TOP)/mk/os161.mkdirs.mk"

# End.
