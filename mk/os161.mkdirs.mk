#
# OS/161 build environment: MKDIRS logic
#
# This generates rules for all intermediate directories as well as
# the directories listed. (That is, if you say MKDIRS=/usr/bin/foo
# you get rules for /usr and /usr/bin as well as /usr/bin/foo.)
#

# Variable for complete list of dirs. Start empty.
_MKDIRS_=

# For each dir in the list...
.for _DIR_ in $(MKDIRS)

#
# Initialize some temporaries.
# _HEAD_ contains / if the path was absolute.
# _INCR_DIR_ accumulates the full directory name incrementally.
#
# Use := to force full evaluation of the RHS of each expression while
# still in the loop, instead of when the variables are used later.
#
_HEAD_:=$(_DIR_:M/*:C/..*/\//)
_INCR_DIR_:=

# For each component in the directory, split on slashes...
.for _COMPONENT_ in $(_DIR_:S/\// /g)

# Add the component to _INCR_DIR_.
_INCR_DIR_:=$(_INCR_DIR_)$(_COMPONENT_)/

# Add the current partial directory to the main list.
# Lose the trailing slash.
_MKDIRS_:=$(_MKDIRS_) $(_HEAD_)$(_INCR_DIR_:S/\/$//)

.endfor # _COMPONENT_
.endfor # _DIR_

#
# Now issue a rule for each directory in the expanded list,
# ordering and uniquifying it.
#
# XXX use /dev/null to suppress curious messages like "../../.. is up
# to date". This scheme probably ought to be reworked.
#
.for _DIR_ in $(_MKDIRS_:O:u)
.if !target($(_DIR_))
$(_DIR_):
.for _PRE_ in $(_DIR_:M*/*:H:N..)
	@$(MAKE) $(_PRE_) >/dev/null 2>&1 || true
.endfor
	mkdir $(_DIR_) || true
.endif
.endfor

# Clear MKDIRS in case we're included again.
MKDIRS=

# End.
