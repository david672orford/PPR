#
# mouse:~ppr/src/ipp/Makefile
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 21 November 2000.
#

include ../makeprogs/global.mk

CGI_PROGS=ipp$(DOTEXE)

PROGS=$(CGI_PROGS)

OBJS_IPP=ipp.$(OBJ) ipp_utils.$(OBJ)

all: $(PROGS)

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(CGI_BIN) $(CGI_PROGS)

ipp$(DOTEXE): $(OBJS_IPP)
	$(LD) -o ipp$(DOTEXE) $(OBJS_IPP) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

# end of file

