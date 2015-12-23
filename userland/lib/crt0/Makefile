#
# Makefile for crt0.o
#

TOP=../../..
.include "$(TOP)/mk/os161.config.mk"

SRCS=$(MACHINE)/crt0.S
MKDIRS=$(INSTALLTOP)/lib $(OSTREE)/lib $(MYBUILDDIR)

.include "$(TOP)/mk/os161.baserules.mk"
.include "$(TOP)/mk/os161.compile.mk"

all-local: $(MYBUILDDIR) .WAIT $(OBJS)

install-staging-local: $(INSTALLTOP)/lib .WAIT
install-local: $(OSTREE)/lib .WAIT

.for O in $(OBJS)
install-staging-local: $(INSTALLTOP)/lib/$(O:T)
$(INSTALLTOP)/lib/$(O:T): $(O)
	cp $(O) $(INSTALLTOP)/lib/$(O:T)

install-local: $(OSTREE)/lib/$(O:T)
$(OSTREE)/lib/$(O:T): $(O)
	cp $(O) $(OSTREE)/lib/$(O:T)
.endfor
