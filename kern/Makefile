#
# Toplevel makefile for the OS/161 kernel tree.
#
# Note: actual kernels are not compiled here; they are compiled in
# compile/FOO where FOO is a kernel configuration name.
#
# We don't actually do anything from here except install includes.
#

TOP=..
.include "$(TOP)/mk/os161.config.mk"

INCLUDES=\
	include/kern include/kern \
	arch/$(MACHINE)/include/kern include/kern/$(MACHINE)

.include "$(TOP)/mk/os161.includes.mk"
