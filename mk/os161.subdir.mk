#
# OS/161 build environment: recurse into subdirectories
#
# Usage:
#    TOP=../..
#    .include "$(TOP)/mk/os161.config.mk"
#    [defs go here]
#    .include "$(TOP)/mk/os161.subdir.mk"
#    [any extra rules go here]
#
# Variables controlling this file:
#
# SUBDIRS			Directories to recurse into.
# EXTRATARGETS			Additional targets to define.
#
# BASETARGETS may also be set to empty to suppress the usual targets.
#
# Note: SUBDIRS may contain .WAIT, which is treated as a parallelism
# barrier like in the right hand side of a make rule.
#
# Further note: if combining os161.subdir.mk with other os161.*.mk
# files (other than os161.config.mk), include os161.subdir.mk first;
# then the ordering of things will cause recursion to happen
# first. Also, .WAIT is inserted so that the recursion will finish
# before anything else happens, which is almost always desirable.
#

BASETARGETS?=\
	all depend install install-staging clean distclean tags \
	cleandir stage dependall build rebuild fullrebuild

# first, make each target depend on its -subdirs target,
# and declare both PHONY.
.for _T_ in $(BASETARGETS) $(EXTRATARGETS)
$(_T_): $(_T_)-subdirs .WAIT
.PHONY: $(_T_) $(_T_)-subdirs
.endfor

# now, make each -subdirs target depend on a rule for each subdir.
.for _D_ in $(SUBDIRS)
.for _T_ in $(BASETARGETS) $(EXTRATARGETS)
.if "$(_D_)" == ".WAIT"
$(_T_)-subdirs: .WAIT
.else
$(_T_)-subdirs: $(_T_)-subdirs-$(_D_)
.PHONY: $(_T_)-subdirs-$(_D_)
$(_T_)-subdirs-$(_D_):
	(cd $(_D_) && $(MAKE) $(_T_))
.endif
.endfor
.endfor
