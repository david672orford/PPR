#
# mouse:~ppr/src/docs/pprhack/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 21 March 2002.
#

# Pull in settings from Configure, the PPR version number, and special rules
# for building documentation.
include ../../Makefile.conf
include ../../include/version.mk
include ../rules.mk

#=== Inventory ==============================================================

BUILT=pprhack.html pprhack.ps pprhack.pdf

#=== Build ==================================================================

all-docs: $(BUILT)

#=== Install ================================================================

install: all-docs
	../$(MAKE_NEW_DIR) $(DOCSDIR)/pprhack
	../$(INSTALLDATA) $(DOCSDIR)/pprhack $(BUILT)

#=== Housekeeping ===========================================================

clean:
	$(RMF) *.$(OBJ) $(BACKUPS)
	$(RMF) *.dvi *.aux *.log *.toc *.tex

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) $(BUILT)

# end of file
