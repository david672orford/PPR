#
# mouse:~ppr/src/ttfutils/Makefile
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
# Last modified 29 June 2000.
#

include ../makeprogs/global.mk

PROGS=ppr-ttf2ps$(DOTEXE) ppr-indexfonts$(DOTEXE)

all: $(PROGS)

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(PROGS)

include .depend

ppr-ttf2ps$(DOTEXE): ppr-ttf2ps.$(OBJ) ../libttf.$(LIBEXT)
	$(CC) $(CFLAGS) -o ppr-ttf2ps$(DOTEXE) ppr-ttf2ps.$(OBJ) ../libttf.$(LIBEXT) $(LIBINTL)

ppr-indexfonts$(DOTEXE): ppr-indexfonts.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT) ../libttf.$(LIBEXT)
	$(CC) $(CFLAGS) -o ppr-indexfonts$(DOTEXE) ppr-indexfonts.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT) ../libttf.$(LIBEXT) $(LIBINTL)

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS)

# end of file
