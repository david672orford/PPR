#
# mouse:~ppr/src/commentators/Makefile
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
# Last modified 4 May 2001.
#

include ../Makefile.conf

PROGS=audio samba xwin pager write mail

all: $(PROGS)

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/commentators $(PROGS)

depend:

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

# end of file
