# Makefile for badcall

TOP=../../..
.include "$(TOP)/mk/os161.config.mk"

PROG=randcall
SRCS=$(MYBUILDDIR)/calls.c main.c
BINDIR=/testbin

CFLAGS+=-I.

.include "$(TOP)/mk/os161.prog.mk"

$(MYBUILDDIR)/calls.c: gencalls.sh callspecs.txt
	./gencalls.sh callspecs.txt > $(MYBUILDDIR)/calls.c

predepend:
	$(MAKE) $(MYBUILDDIR)/calls.c

clean: myclean
myclean:
	rm -f $(MYBUILDDIR)/calls.c

.PHONY: predepend myclean
