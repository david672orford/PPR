#
# mouse:~ppr/src/docs/pprdoc/Makefile
# Copyright 1995--2003, Trinity College Computing Center.
# Written by David Chappell.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
#
# Last modified 14 October 2003.
#

# Pull in settings from Configure, the PPR version number, and special rules
# for building documentation.
include ../../Makefile.conf
include ../../makeprogs/paths.mk
include ../rules.mk

#=== Inventory ==============================================================

BUILT=\
	pprdoc.html \
	pprdoc.pdf \
	pprdoc-enter.png pprdoc-printing1.png pprdoc-printing2.png \
	pprdoc-enter.eps pprdoc-printing1.eps pprdoc-printing2.eps \
	
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
	$(RMF) *.dvi *.aux *.log *.toc *.tex *.ps

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) $(BUILT)

# end of file
