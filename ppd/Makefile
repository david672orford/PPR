#
# mouse:~ppr/src/ppd/Makefile
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

all:
	( cd ghostscript; $(MAKE) )

# Copy the files in each vendor's directory into the final
# destination PPD files directory.
install:
	( cd ghostscript; $(MAKE) install )
	$(INSTALLDATA) $(SHAREDIR)/PPDFiles generic/* other/*
	$(INSTALLDATA) $(SHAREDIR)/lib pagesizes.conf

depend:

clean:
	$(RMF) $(BACKUPS)
	( cd ghostscript; $(MAKE) clean )
	( cd generic; $(RMF) $(BACKUPS) )
	( cd other; $(RMF) $(BACKUPS) )

# end of file

