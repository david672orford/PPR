#
# mouse:~ppr/src/docs/pprinst/Makefile
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 7 May 2001.
#

# Pull in settings from Configure, the PPR version number, and special rules
# for building documentation.
include ../../makeprogs/global.mk
include ../../include/version.mk
include ../rules.mk

#=== Inventory ==============================================================

BUILT=pprinst.html pprinst.ps pprinst.pdf

#=== Build ==================================================================

all: $(BUILT)

#=== Install ================================================================

install: all
	../$(MAKE_NEW_DIR) $(DOCSDIR)/pprinst
	../$(INSTALLDATA) $(DOCSDIR)/pprinst $(BUILT)

#=== Housekeeping ===========================================================

# Remove editor backup files and TeX intermediate files but not the built
# documentation.
clean:
	$(RMF) $(BACKUPS)
	$(RMF) *.dvi *.aux *.log *.toc *.tex

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) $(BUILT)

# end of file

