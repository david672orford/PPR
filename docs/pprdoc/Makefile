#
# mouse:~ppr/src/docs/pprdoc/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 24 January 2002.
#

# Pull in settings from Configure, the PPR version number, and special rules
# for building documentation.
include ../../Makefile.conf
include ../../include/version.mk
include ../rules.mk

#=== Inventory ==============================================================

BUILT=pprdoc.html pprdoc.ps pprdoc.pdf \
	pprdoc-enter.png pprdoc-printing1.png pprdoc-printing2.png \
	pprdoc-enter.eps pprdoc-printing1.eps pprdoc-printing2.eps

#=== Build ==================================================================

all-docs: $(BUILT)

pprdoc.ps: pprdoc-enter.eps pprdoc-printing1.eps pprdoc-printing2.eps

#=== Validity Check ================================================================

lint:
	SGML_CATALOG_FILES=$(SGML_CATALOG_FILES) $(XMLLINT) --catalogs pprdoc.sgml

#=== Install ================================================================

install: all-docs
	../$(MAKE_NEW_DIR) $(DOCSDIR)/pprdoc
	../$(INSTALLDATA) $(DOCSDIR)/pprdoc $(BUILT)

#=== Housekeeping ===========================================================

# Remove editor backup files and TeX intermediate files.  Notice that
# we let the .eps files live because otherwise make will want to
# rebuild them in order to be sure the PostScript and PDF files
# are up to date.
clean:
	$(RMF) $(BACKUPS)
	$(RMF) *.dvi *.aux *.log *.toc *.tex

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) $(BUILT)

# end of file
