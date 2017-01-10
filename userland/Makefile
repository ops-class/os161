#
# Toplevel userland makefile for OS/161.
#

TOP=..
.include "$(TOP)/mk/os161.config.mk"

# lib must be first.
SUBDIRS=lib .WAIT bin sbin testbin

INCLUDES=\
	include include \
	include/sys include/sys \
	include/test include/test \
	include/test161 include/test161 \
	include/types include/types

INCLUDELINKS=\
	kern/machine include/machine \
	$(MACHINE) include/kern/machine

.include "$(TOP)/mk/os161.subdir.mk"
.include "$(TOP)/mk/os161.includes.mk"

includes: hostincludes
hostincludes:
	(cd lib/hostcompat && $(MAKE) includes)

.PHONY: includes hostincludes
