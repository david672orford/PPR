#
# mouse:~ppr/src/docs/rules.mk
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 11 March 2005.
#

# Where do we install the documentation?
DOCSDIR=$(WWWDIR)/docs

# Where are the document formatting and conversion programs?
POD2MAN=pod2man
POD2HTML=pod2html
FIG2DEV=PATH=/usr/bin:/usr/X11R6/bin:$(PATH) fig2dev
XSLTPROC=/usr/bin/xsltproc
XMLLINT=xmllint
HTMLDOC=htmldoc
FOP=fop

# Where are the style sheets?
XSL_SPEC_HTML=../../nonppr_misc/docbook-xsl/html/docbook.xsl
XSL_SPEC_PRINT=../../nonppr_misc/docbook-xsl/fo/docbook.xsl
SGML_CATALOG_FILES=../../nonppr_misc/docbook-xml/docbook.cat

# Additional file extensions to be used in our rules.
.SUFFIXES: .pod .sgml .fo .html .man .eps .ps .pdf .fig .gif .jpeg .png

#============================================================================
# Rule to convert POD to HTML using Perl's pod2html
#============================================================================
.pod.html:
	$(POD2HTML) --htmlroot=. --podpath=.:../refman --libpods=$(LIBPODS) $*.pod >$*.html

#============================================================================
# Rule to convert POD to Nroff format using Perl's pod2man
#============================================================================
.pod.man:
	NAME=`perl -e '$$ARGV[0] =~ s/\.[0-9]$$//; print $$ARGV[0];' $*`; \
	ln $*.pod $$NAME.pod; \
	$(POD2MAN) --center="PPR Documentation" --release=$(SHORT_VERSION) \
		--section=`perl -e '$$ARGV[0] =~ /([0-9])$$/; print $$1;' $*` \
		$$NAME.pod >$*.man; \
	rm $$NAME.pod

#============================================================================
# Rules to convert Docbook SGML to HTML using Xsltproc
#============================================================================

.sgml.html:
	SGML_CATALOG_FILES=$(SGML_CATALOG_FILES) $(XSLTPROC) --catalogs --nonet --output $*.html $(XSL_SPEC_HTML) $*.sgml

#============================================================================
# Rules to convert HTML to PostScript and PDF using HTMLDOC
#============================================================================

.html.ps:
	-$(HTMLDOC) --no-toc --browserwidth 1024 -t ps --outfile $*.ps $*.html

.html.pdf:
	-$(HTMLDOC) --no-toc --browserwidth 1024 -t pdf12 --compression=9 --outfile $*.pdf $*.html

#============================================================================
# Rules to convert Docbook SGML to PostScript and PDF by way
# of XML Formatted Objects
#
# As of the 17 January 2003 CVS version, FOP does a poor job, overprinting
# some lines.
#============================================================================

.sgml.fo:
	SGML_CATALOG_FILES=$(SGML_CATALOG_FILES) $(XSLTPROC) --catalogs --docbook --nonet --output $*.fo $(XSL_SPEC_PRINT) $*.sgml

#.fo.ps:
#	$(FOP) -fo $*.fo -ps $*.ps

#.fo.pdf:
#	$(FOP) -fo $*.fo -pdf $*.pdf

#============================================================================
# Rules to convert Xfig files to various vector and bitmap formats.
#============================================================================

.fig.eps:
	$(FIG2DEV) -L ps $*.fig $*.eps || ( rm -f $*.eps; exit 1 )

.fig.gif:
	$(FIG2DEV) -L gif $*.fig $*.gif || ( rm -f $*.gif; exit 1 )

.fig.jpeg:
	$(FIG2DEV) -L jpeg $*.fig >$*.jpeg || ( rm -f $*.jpeg; exit 1 )

.fig.png:
	$(FIG2DEV) -L png $*.fig >$*.png || ( rm -f $*.png; exit 1 )

# end of file

