#
# mouse:~ppr/src/docs/Makefile
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

BUILT=pprdoc.html pprdoc.ps pprdoc.pdf \
	pprdoc-enter.png pprdoc-printing1.png pprdoc-printing2.png

#=== Build ==================================================================

all: $(BUILT)

pprdoc.ps: pprdoc-enter.eps pprdoc-printing1.eps pprdoc-printing2.eps

#=== Install ================================================================

install: all
	../$(MAKE_NEW_DIR) $(DOCSDIR)/pprdoc
	../$(INSTALLDATA) $(DOCSDIR)/pprdoc $(BUILT)

#=== Housekeeping ===========================================================

# Remove editor backup files and TeX intermediate files.
clean:
	$(RMF) $(BACKUPS)
	$(RMF) *.dvi *.aux *.log *.toc *.tex *.eps

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) $(BUILT)

# end of file
