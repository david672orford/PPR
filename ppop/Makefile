#
# mouse:~ppr/src/ppop/Makefile
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
# Last modified 10 February 2001.
#

include ../Makefile.conf

#=== Inventory ===================================================================

BIN_PROGS=ppop$(DOTEXE)

LIB_PROGS=

#=== Build =======================================================================

all: $(BIN_PROGS) $(LIB_PROGS)

OBJS1=\
	ppop.$(OBJ) \
	ppop_cmds_listq.$(OBJ) \
	ppop_cmds_other.$(OBJ) \
	ppop_modify.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
ppop$(DOTEXE): $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(INTLLIBS)
	$(CHMOD) 4755 $@

#=== Install =====================================================================

install: $(BIN_PROGS) $(LIB_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6711 $(HOMEDIR)/bin $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6711 $(HOMEDIR)/lib $(LIB_PROGS)

#=== Housekeeping ================================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(BIN_PROGS) $(LIB_PROGS)

# end of file

