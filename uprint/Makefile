#
# mouse:~ppr/src/uprint/Makefile
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

PROGS= uprint-lpr$(DOTEXE) uprint-lpq$(DOTEXE) uprint-lprm$(DOTEXE) \
	uprint-lp$(DOTEXE) uprint-cancel$(DOTEXE) uprint-lpstat$(DOTEXE)

PRIVATE_LIBS=../libuprint.$(LIBEXT) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)

PUBLIC_LIBS=$(SOCKLIBS) $(INTLLIBS)

LINK_LIBS=$(PRIVATE_LIBS) $(PUBLIC_LIBS)

#=== Build ==================================================================

all: $(PROGS)

uprint-lp$(DOTEXE): uprint-lp.$(OBJ) $(PRIVATE_LIBS)
	$(CC) $(CFLAGS) -o $@ uprint-lp.$(OBJ) $(LINK_LIBS)

uprint-lpr$(DOTEXE): uprint-lpr.$(OBJ) $(PRIVATE_LIBS)
	$(CC) $(CFLAGS) -o $@ uprint-lpr.$(OBJ) $(LINK_LIBS)

uprint-lpq$(DOTEXE): uprint-lpq.$(OBJ) $(PRIVATE_LIBS)
	$(CC) $(CFLAGS) -o $@ uprint-lpq.$(OBJ) $(LINK_LIBS)

uprint-lprm$(DOTEXE): uprint-lprm.$(OBJ) $(PRIVATE_LIBS)
	$(CC) $(CFLAGS) -o $@ uprint-lprm.$(OBJ) $(LINK_LIBS)

uprint-cancel$(DOTEXE): uprint-cancel.$(OBJ) $(PRIVATE_LIBS)
	$(CC) $(CFLAGS) -o $@ uprint-cancel.$(OBJ) $(LINK_LIBS)

uprint-lpstat$(DOTEXE): uprint-lpstat.$(OBJ) $(PRIVATE_LIBS)
	$(CC) $(CFLAGS) -o $@ uprint-lpstat.$(OBJ) $(LINK_LIBS)

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) root $(GROUP_PPR) 4755 $(HOMEDIR)/bin $(PROGS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS)

# end of file
