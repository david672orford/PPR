#
# mouse:~ppr/src/templates/Makefile
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

BIN_PROGS=

LIB_PROGS=

#=== Build ==================================================================

all: $(BIN_PROGS) $(LIB_PROGS)

OBJS=\
	xxx.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
xxx$(DOTEXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

#=== Install ================================================================

install: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib $(LIB_PROGS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(BIN_PROGS)

# end of file

