#
# mouse:~ppr/src/ppad/Makefile
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

include ../Makefile.conf

#=== Inventory ==============================================================

BIN_PROGS=ppad$(DOTEXE)

#=== Build ==================================================================

all: $(BIN_PROGS)

OBJS1=ppad.$(OBJ) \
	ppad_printer.$(OBJ) \
	ppad_media.$(OBJ) \
	ppad_group.$(OBJ) \
	ppad_conf.$(OBJ) \
	ppad_filt.$(OBJ) \
	ppad_util.$(OBJ) \
	ppad_alias.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
ppad$(DOTEXE): $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(INTLLIBS)
	chmod 4755 $@

#=== Install ================================================================

install: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6755 $(HOMEDIR)/bin $(BIN_PROGS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(BIN_PROGS)

# end of file

