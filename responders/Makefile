#
# mouse:~ppr/src/responders/Makefile
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
# Last modified 22 November 2000.
#

include ../makeprogs/global.mk

PROGS=write netsend mail samba xwin atalk pprpopup audio

all: $(PROGS)

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/responders $(PROGS)
	$(INSTALLLN) $(HOMEDIR)/responders/mail $(HOMEDIR)/responders/errmail

depend:

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

# end of file

