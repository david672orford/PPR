#
# mouse:~ppr/src/docs/rules.mk
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 21 June 2001.
#

# Where do we install the documentation?
DOCSDIR=$(WWWDIR)/docs

# Where are the document formatting and conversion programs?
POD2MAN=pod2man
POD2HTML=pod2html
FIG2DEV=fig2dev
JADE=jade
JADETEX=jadetex
DVIPS=dvips
PS2PDF=ps2pdf

# Where are the style sheets?
DSSSL_SPEC_HTML=/usr/share/sgml/docbook/dsssl-stylesheets/html/docbook.dsl
DSSSL_SPEC_PRINT=/usr/share/sgml/docbook/dsssl-stylesheets/print/docbook.dsl

# Additional file extensions to be used in our rules.
.SUFFIXES: .pod .html .man .dvi .tex .eps .ps .pdf .sgml .fig .gif .jpeg .png

# Rule to create an HTML file from a Perl POD file
.pod.html:
	$(POD2HTML) $*.pod >$*.html

# Rule to convert POD to nroff format
.pod.man:
	NAME=`perl -e '$$ARGV[0] =~ s/\.[0-9]$$//; print $$ARGV[0];' $*`; \
	ln $*.pod $$NAME.pod; \
	$(POD2MAN) --center="PPR Documentation" --release=$(VERSION) \
		--section=`perl -e '$$ARGV[0] =~ /([0-9])$$/; print $$1;' $*` \
		$$NAME.pod >$*.man; \
	rm $$NAME.pod

.sgml.tex:
	$(JADE) -t tex -d $(DSSSL_SPEC_PRINT) -i tex $*.sgml

.tex.ps:
	$(JADETEX) $*; \
		while grep 'LaTeX Warning: Label(s) may have changed' $*.log >/dev/null; \
		do $(JADETEX) $*; done
	$(DVIPS) -f $* >$*.ps

.ps.pdf:
	$(PS2PDF) $*.ps $*.pdf

.sgml.html:
	$(JADE) -t sgml -i html -V nochunks -d $(DSSSL_SPEC_HTML) $*.sgml \
		| sed -e 's/SRC="\([^"]*\)"/SRC="\1.png"/' >$*.html

.fig.eps:
	$(FIG2DEV) -L ps $*.fig $*.eps

.fig.gif:
	$(FIG2DEV) -L gif $*.fig $*.gif

.fig.jpeg:
	$(FIG2DEV) -L jpeg $*.fig >$*.jpeg

.fig.png:
	$(FIG2DEV) -L png $*.fig >$*.png

# end of file

