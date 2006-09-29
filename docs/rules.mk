#
# mouse:~ppr/src/docs/rules.mk
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 29 September 2006.
#

# Where do we install the documentation?
DOCSDIR=$(WWWDIR)/docs

# Where are the document formatting and conversion programs?
POD2MAN=pod2man
POD2HTML=pod2html
FIG2DEV=PATH=/usr/bin:/usr/X11R6/bin:$(PATH) fig2dev
XSLTPROC=xsltproc
XMLLINT=xmllint
HTMLDOC=htmldoc
FOP=fop

# Where are the style sheets?
XSL_SPEC_HTML=../../nonppr_misc/docbook-xsl/html/docbook.xsl
XSL_SPEC_PRINT=../../nonppr_misc/docbook-xsl/fo/docbook.xsl
XSL_SPEC_MAN=../../nonppr_misc/docbook-xsl/manpages/docbook.xsl
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
# Rules to convert Docbook SGML to HTML  and Nroff using Xsltproc
#============================================================================

.sgml.html:
	SGML_CATALOG_FILES=$(SGML_CATALOG_FILES) $(XSLTPROC) --catalogs --nonet --output $*.html $(XSL_SPEC_HTML) $*.sgml

.sgml.man:
	SGML_CATALOG_FILES=$(SGML_CATALOG_FILES) $(XSLTPROC) --catalogs --nonet --output $*.man $(XSL_SPEC_MAN) $*.sgml
	mv $* $*.man

#============================================================================
# Rules to convert HTML to PostScript and PDF using HTMLDOC
#============================================================================

.html.ps:
	$(HTMLDOC) --no-toc --browserwidth 1024 -t ps --outfile $*.ps $*.html

.html.pdf:
	$(HTMLDOC) --no-toc --browserwidth 1024 -t pdf12 --compression=9 --outfile $*.pdf $*.html

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

