#
# mouse:~ppr/src/ppop/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 23 April 2006.
#

include ../Makefile.conf

#=== Inventory ===================================================================

BIN_PROGS=ppop$(DOTEXE)

#=== Build =======================================================================

all: $(BIN_PROGS) $(LIB_PROGS)

ppop$(DOTEXE): \
		ppop.o \
		ppop_cmds_listq.o \
		ppop_cmds_other.o \
		ppop_modify.o \
		../libppr.a \
		../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)
	$(CHMOD) 4755 $@

#=== Install =====================================================================

install: $(BIN_PROGS) $(LIB_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6711 $(BINDIR) $(BIN_PROGS)

#=== Housekeeping ================================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.o $(BIN_PROGS) $(LIB_PROGS)

# end of file

