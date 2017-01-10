#
# OS/161 build environment base definitions
#
# Proper usage:
#    TOP=../..		# or however many levels needed
#    .include "$(TOP)/mk/os161.config.mk"
#
# This file takes care of including $(TOP)/defs.mk and sets defaults
# for things that can be defined there.
#
# It and defs.mk collaboratively set various make variables that
# control the build environment.
#
############################################################
#
# These build variables are meant to be user-settable:
#
# (Locations.)
#
# OSTREE			The root directory you run OS/161 in.
#				Default is ~/os161/root.
#
# WORKDIR			Top of a tree to use for compiling.
#                               Default is $(TOP)/build.
#
# BUILDSYMLINKS			If set to "yes", symlinks in each directory
#				will be made to point into $(BUILDTOP).
#				Default is "yes".
#
# By default the system will be built within the source tree and
# installed to ~/os161/root. It is expected this will be sufficient
# for most uses. If your root directory is somewhere else, set OSTREE
# in defs.mk. If you're running on a large computing cluster with
# networked home directories, setting WORKDIR to point somewhere on a
# local disk will probably make your builds quite a bit faster, at the
# cost of having to recompile if you switch to a different machine.
# If you want the source tree to be completely read-only, which is
# occasionally useful for tracking down build glitches, you can set
# WORKDIR and also set BUILDSYMLINKS to no.
#
# (Platform.)
#
# PLATFORM			The type of system we're building for.
#				Should always be set by defs.mk.
#
# MACHINE			The processor type we're building for.
#				Should always be set by defs.mk.
#
# The target machine type is set when you configure the source tree.
# If you change it, be sure to make distclean and recompile everything.
# Not all possible combinations of PLATFORM and MACHINE are allowed.
# See the logic at the bottom of this file for a list of supported
# combinations. If you are trying to port OS/161 to a new machine, the
# first step is to update that list.
#
# (Compilation.)
#
# DEBUG				Compiler option for debug vs. optimize.
#				Default: -O2
#
# WARNINGS			Compiler options for warnings.
#				Default: -Wall -Wextra -Wwrite-strings
#					 -Wmissing-prototypes
#
# WERROR			Compiler option to make warnings fatal.
#				Default: -Werror
#
# Since debugging of user-level programs is not supported in OS/161
# (and not really supportable without a lot of work on your part)
# there's usually not much reason to change DEBUG. If you want to,
# however, the easiest way is to usually set it on the make command
# line:
#    make clean
#    make DEBUG=-g all
# recompiles the current directory with debug info.
#
# Similarly, if you have a lot of warnings and you want to temporarily
# ignore them while you fix more serious problems, you can turn off
# the error-on-warning behavior on the fly by setting WERROR to empty:
#    make WERROR=
#
# This convenience is why these variables are separately defined
# rather than just being rolled into CFLAGS.
#
############################################################
#
# These build variables can be set explicitly for further control if
# desired, but should in general not need attention.
#
# (Locations.)
#
# BUILDTOP			Top of tree where .o files go.
#				Default is $(WORKDIR).
#
# TOOLDIR			Place for compiled programs used in
#				the build. Default is $(WORKDIR)/tooldir.
#
# INSTALLTOP			Staging directory for installation.
#				Default is $(WORKDIR)/install.
#
# Probably the only reason to change these would be if you're short on
# diskspace in $(WORKDIR).
#
# (Platform.)
#
# GNUTARGET			The GNU gcc/binutils name for the
#				target we're building for.
#				Defaults to $(MACHINE)-harvard-os161.
#
# This should not need to be changed.
#
# (Programs.)
#
# CC				(Cross-)compiler.
#				Default is $(GNUTARGET)-gcc.
#
# LDCC				(Cross-)compiler when invoked for linking.
#				Default is $(CC).
#
# AS				(Cross-)assembler.
#				Default is $(GNUTARGET)-as.
#
# LD				(Cross-)linker.
#				Default is $(GNUTARGET)-ld.
#
# AR				Archiver (librarian).
#				Default is $(GNUTARGET)-ar.
#
# RANLIB			Library postprocessor/indexer.
#				Default is $(GNUTARGET)-ranlib.
#
# NM				Tool to print symbol tables.
#				Default is $(GNUTARGET)-nm.
#
# SIZE				Tool to print sizes of binaries.
#				Default is $(GNUTARGET)-size.
#
# STRIP				Tool to remove debugging information.
#				Default is $(GNUTARGET)-strip.
#
# The above are the compilation tools for OS/161. They create programs
# that run on OS/161. Since OS/161 is not meant to be self-hosting,
# programs that we need to run during the build, or run manually
# outside of the machine simulator, need to be compiled with a
# different compiler that creates programs that run for the "host" OS,
# whatever that is (Linux, NetBSD, FreeBSD, MacOS X, Solaris, etc.)
# The host compilation tools are prefixed with HOST_ as follows:
#
#
# HOST_CC			Host compiler.
#				Default is gcc.
#
# HOST_LDCC			Host compiler when invoked for linking.
#				Default is $(HOST_CC).
#
# HOST_AS			Host assembler.
#				Default is as.
#
# HOST_LD			Host linker.
#				Default is ld.
#
# HOST_AR			Host archiver (librarian).
#				Default is ar.
#
# HOST_RANLIB			Host library postprocessor/indexer.
#				Default is ranlib.
#
# HOST_NM			Host tool to print symbol tables.
#				Default is nm.
#
# HOST_SIZE			Host tool to print sizes of binaries.
#				Default is size.
#
# HOST_STRIP			Host tool to remove debugging information.
#				Default is strip.
#
# In general there should be no need to change the cross-compiler
# variables, unless you are e.g. trying to build OS/161 with something
# other than gcc, or you want to supply an explicit location for a
# specific copy of gcc somewhere rather than use the one on your
# $PATH.
#
# However, on some systems it might conceivably be necessary to change
# the host tool variables. For example, there are some (now extremely
# old) systems where "ranlib" is not only not needed but also corrupts
# libraries, in which case you might set HOST_RANLIB=true. If your
# host machine doesn't have "gcc" you may be able to set HOST_CC=cc
# and fiddle with HOST_WARNINGS and HOST_CFLAGS (below) and have
# things still more or less work.
#
# (Compilation.)
#
# HOST_DEBUG			Like DEBUG, but for programs to be
#				built to run on the host OS.
#				Default is $(DEBUG).
#
# HOST_WARNINGS			Like WARNINGS, but for programs to be
#				built to run on the host OS.
#				Default is $(WARNINGS).
#
# HOST_WERROR			Like WERROR, but for programs to be
#				built to run on the host OS.
#				Default is $(WERROR).
#
############################################################
#
# These build variables should probably not be tinkered with in
# defs.mk and serve as baseline values to be added to by individual
# program makefiles and used by other os161.*.mk files.
#
# (Compilation.)
#
# CFLAGS			Full baseline compile flags.
#				Default is $(DEBUG) $(WARNINGS) $(WERROR),
#				plus what's needed for -nostdinc.
#
# KCFLAGS			Like CFLAGS, but for the kernel, which
#				configures debug/optimize separately.
#				Default is $(KDEBUG) $(WARNINGS) $(WERROR).
#				(KDEBUG is set by the kernel config script.)
#
# HOST_CFLAGS			Like CFLAGS, but for programs to be
#				built to run on the host OS. Default is
#				$(HOST_DEBUG) $(HOST_WARNINGS) $(HOST_WERROR).
#
# LDFLAGS			Baseline link-time flags.
#				Default is empty plus what's needed for
#				-nostdlib.
#
# KLDFLAGS			Baseline link-time flags for kernel.
#				Default is empty.
#
# HOST_LDFLAGS			Like LDFLAGS, but for programs to be
#				built to run on the host OS.
#				Default is empty.
#
# LIBS				Baseline list of libraries to link to.
#				Default is empty plus what's needed for
#				-nostdlib.
#
# HOST_LIBS			Like LIBS, but for programs to be
#				built to run on the host OS.
#				Default is empty.
#
############################################################
#
# These variables should not be changed directly, or by individual
# program makefiles either, and are for use by other os161.*.mk files.
#
# (Locations.)
#
# MYDIR				Name of current source directory, relative
#				to $(TOP); e.g. bin/sh.
#
# MYBUILDDIR			Build directory for current source directory.
#
# MKDIRS			Directories to create, mostly for installing.
#
# ABSTOP_PATTERN, ABSTOP	Private, used to compute other locations.
#
# (Compilation.)
#
# MORECFLAGS			Same as CFLAGS but comes later on the
#				compile command line. In general individual
#				makefiles shouldn't touch this.
#				Default is empty plus what's needed for
#				-nostdinc.
#
# MORELIBS			Same as LIBS but comes after on the link
#				line. In general individual makefiles
#				shouldn't touch this.
#				Default is empty plus what's needed for
#				-nostdlib.
#
############################################################

# Some further vars that currently exist but should be moved
# around elsewhere:
#
# COMPAT_CFLAGS
# COMPAT_TARGETS


############################################################
# Establish defaults.
# (Variables are set in the order documented above.)
#
# These definitions are set firmly here (that is, not with ?=) because
# some of them, like CC, are predefined by make. Because defs.mk is
# included afterwards it can override any of these settings.
#

#
# User-settable configuration
#

# Locations of things.
OSTREE=$(HOME)/os161/root	# Root directory to install into.
WORKDIR=$(TOP)/build		# Top of tree to build into.
BUILDSYMLINKS=yes		# yes => link build -> $(BUILDTOP)/$(HERE).

# Platform we're building for.
PLATFORM=sys161
MACHINE=mips

# Compilation
DEBUG=-O2
WARNINGS=-Wall -W -Wwrite-strings -Wmissing-prototypes
WERROR=-Werror

#
# Less-likely-to-need-setting
#

# Locations of things.
BUILDTOP=$(WORKDIR)		# Top of directory for compiler output.
TOOLDIR=$(WORKDIR)/tooldir	# Place for host progs used in the build.
INSTALLTOP=$(WORKDIR)/install	# Staging area for installation.

# Platform.
GNUTARGET=$(MACHINE)-harvard-os161

# Programs and tools.
CC=$(GNUTARGET)-gcc		# Compiler.
LDCC=$(CC)			# Compiler when used for linking.
AS=$(GNUTARGET)-as		# Assembler.
LD=$(GNUTARGET)-ld		# Linker.
AR=$(GNUTARGET)-ar		# Archiver.
RANLIB=$(GNUTARGET)-ranlib	# Library indexer.
NM=$(GNUTARGET)-nm		# Symbol dumper.
SIZE=$(GNUTARGET)-size		# Size tool.
STRIP=$(GNUTARGET)-strip	# Debug strip tool.
HOST_CC=gcc			# Host compiler.
HOST_LDCC=$(HOST_CC)		# Host compiler when used for linking.
HOST_AS=as			# Host assembler.
HOST_LD=ld			# Host linker.
HOST_AR=ar			# Host archiver.
HOST_RANLIB=ranlib		# Host library indexer.
HOST_NM=nm			# Host symbol dumper.
HOST_SIZE=size			# Host size tool.
HOST_STRIP=strip		# Host debug strip tool.

# Compilation.
HOST_DEBUG=$(DEBUG)
HOST_WARNINGS=$(WARNINGS)
HOST_WERROR=$(WERROR)

#
# Probably-shouldn't-be-touched
#

CFLAGS=$(DEBUG) $(WARNINGS) $(WERROR) -std=gnu99
KCFLAGS=$(KDEBUG) $(WARNINGS) $(WERROR) -std=gnu99
HOST_CFLAGS=$(HOST_DEBUG) $(HOST_WARNINGS) $(HOST_WERROR) \
	-I$(INSTALLTOP)/hostinclude

LDFLAGS=
KLDFLAGS=
HOST_LDFLAGS=

LIBS=
HOST_LIBS=

#
# Don't touch.
#

MORECFLAGS=
MORELIBS=

# lib/hostcompat.
COMPAT_CFLAGS=
COMPAT_TARGETS=

############################################################
# Get defs.mk to get the real configuration for this tree.
# If it doesn't exist, we'll go with the defaults.

.-include "$(TOP)/defs.mk"

############################################################
# Make sure we have a supported PLATFORM and MACHINE.

# We support mips on system/161.
SUPPORTED_TARGETS=sys161 mips

_OK_=0
.for _P_ _M_ in $(SUPPORTED_TARGETS)
.if "$(PLATFORM)" == "$(_P_)" && "$(MACHINE)" == "$(_M_)"
_OK_=1
.endif
.endfor

.if "$(_OK_)" != "1"
.init:
	@echo "Platform $(PLATFORM) and machine $(MACHINE) not supported"
	@false
.endif

############################################################
# Get any machine-dependent flags or makefile definitions

.-include "$(TOP)/mk/os161.config-$(MACHINE).mk"

############################################################
# Establish some derived locations.

#
# Absolute location of the top of the source tree. This should only be
# used to un-absolutize other paths; the tree ought to be independent
# of where it happens to live.
#
# This works by turning $(TOP) into a regexp and substituting it into
# the current directory. Note that it doesn't escape all regexp
# metacharacters -- if you make a directory named "foo*bar" or
# something you deserve the consequences.
#
# .CURDIR is a make builtin.
#
ABSTOP_PATTERN=$(TOP:S/./\\./g:S/\\.\\./[^\/]*/g)
ABSTOP=$(.CURDIR:C/$(ABSTOP_PATTERN)\$//)

# Find the name of the current directory relative to TOP.
# This works by removing ABSTOP from the front of .CURDIR.
MYDIR=$(.CURDIR:S/^$(ABSTOP)//)

# Find the build directory corresponding to the current source dir.
.if "$(MYDIR)" == ""
# avoid stray slash
MYBUILDDIR=$(BUILDTOP)
.else
MYBUILDDIR=$(BUILDTOP)/$(MYDIR)
.endif

############################################################
# Ensure we compile a consistent tree.

#
# Traditionally in Unix the first step of recompiling the system is to
# install new header files. Furthermore, the second step is to compile
# and install new libraries, before continuing on to the rest of the
# OS, which can then be compiled with those new headers and new
# libraries.
#
# Combining the compile and install phases like this is simpler and
# uses less disk space on extra copies of things (which mattered, back
# in the day) but has a number of problems. Chief among these is that
# if the build bombs out halfway through you end up with a partly
# updated and maybe broken system. It also means that once you start
# recompiling the system you can't easily back out. And the behavior
# violates the principle of least surprise.
#
# OS/161 1.x had, intentionally, a very traditional build environment.
# In OS/161 2.x, however, we use a staging area to avoid mixing build
# and install. This means that we must compile only against the
# staging area, $(INSTALLTOP), and never use the headers or libraries
# installed in $(OSTREE) until install time.
#
# This means that regardless of whether we have a gcc configured so it
# includes from our $(OSTREE) by default or not, we must use -nostdinc
# and -nostdlib and explicitly link with materials from $(INSTALLTOP).
#
# Use MORECFLAGS and MORELIBS, which are supported by os161.compile.mk
# for this purpose, so the include paths and library list come out in
# the right order.
#

CFLAGS+=-nostdinc
MORECFLAGS+=-I$(INSTALLTOP)/include
LDFLAGS+=-nostdlib -L$(INSTALLTOP)/lib $(INSTALLTOP)/lib/crt0.o
MORELIBS+=-lc
LIBDEPS+=$(INSTALLTOP)/lib/crt0.o $(INSTALLTOP)/lib/libc.a
LIBS+=-ltest161
############################################################

# end.
