#
# mouse:~ppr/src/ppd/Makefile
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
# Last modified 18 July 2001.
#

include ../Makefile.conf

#=== Inventory ==============================================================

#=== Build ==================================================================

all:
	( cd ghostscript; $(MAKE) )
	( cd generic; $(MAKE) )

#=== Install ================================================================

install:
	( cd ghostscript; $(MAKE) install )
	$(INSTALLDATA) $(SHAREDIR)/PPDFiles other/*
	( cd generic; $(MAKE) install )
	$(INSTALLDATA) $(SHAREDIR)/lib pagesizes.conf

#=== Housekeeping ===========================================================

depend:

clean:
	$(RMF) $(BACKUPS)
	( cd ghostscript; $(MAKE) clean )
	( cd generic; $(MAKE) clean )
	( cd other; $(RMF) $(BACKUPS) )

# end of file

