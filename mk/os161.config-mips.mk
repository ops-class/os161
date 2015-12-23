#
# OS/161 build environment extra definitions for MIPS.
# This is included by os161.config.mk.
#


#
# The MIPS toolchain for OS/161 is an ELF toolchain that by default
# generates SVR4 ELF ABI complaint code. This means that by default
# all code is PIC (position-independent code), which is all very well
# but not something we need or want in the kernel. So we use -fno-pic
# to turn this behavior off.
#
# We turn it off for userland too because we don't support shared
# libraries. Before trying to implement shared libraries, these
# options need to be taken out of CFLAGS.
#
# It turns out you also need -mno-abicalls to turn it off completely.
#

CFLAGS+=-mno-abicalls -fno-pic
KCFLAGS+=-mno-abicalls -fno-pic

#
# Extra stuff required for the kernel.
#
# -ffixed-23 reserves register 23 (s7) to hold curthread. This register
# number must match the curthread definition in mips/thread.h and the
# code in trap.S.
#
KCFLAGS+=-ffixed-23
KLDFLAGS+=
