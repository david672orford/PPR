#
# mouse:~ppr/src/filter_lp/Makefile
# Copyright 1995--2000, Trinity College Computing Center
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 30 June 2000.
#

include ../makeprogs/global.mk

PROGS=filter_lp$(DOTEXE)

all: $(PROGS)

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/filters filter_lp$(DOTEXE)
	$(INSTALLLN) $(HOMEDIR)/filters/filter_lp$(DOTEXE) $(HOMEDIR)/filters/filter_lp_autolf$(DOTEXE)
	$(INSTALLLN) $(HOMEDIR)/filters/filter_lp$(DOTEXE) $(HOMEDIR)/filters/filter_fortran$(DOTEXE)

include .depend

filter_lp$(DOTEXE): filter_lp.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(CC) $(CFLAGS) -o filter_lp$(DOTEXT) filter_lp.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file
