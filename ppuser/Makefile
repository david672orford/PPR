#
# mouse:~ppr/src/ppuser/Makefile
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 11 February 2001.
#

include ../makeprogs/global.mk

#=== Inventory ==============================================================

BIN_PROGS=ppuser$(DOTEXE)

#=== Build ==================================================================

all: $(BIN_PROGS)

OBJS=ppuser.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT) \
	../libpprdb.$(LIBEXT)
ppuser$(DOTEXE): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(DBLIBS) $(INTLLIBS)
	$(CHMOD) 4755 $@

#=== Install ================================================================

install: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6711 $(HOMEDIR)/bin $(BIN_PROGS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(BIN_PROGS)

# end of file

