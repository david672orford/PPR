#
# mouse:~ppr/src/docs/uprint/Makefile
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
# Last modified 16 February 2001.
#

# Pull in settings from Configure.
include ../../makeprogs/global.mk
include ../../include/version.mk
include ../rules.mk

#=== Inventory ==============================================================

# List of man pages in HTML format
ALL_HTML=UPRINT.1.html uprint-remote.conf.5.html \
	uprint-lp.1.html uprint-lpstat.1.html uprint-cancel.1.html \
	uprint-lpr.1.html uprint-lpq.1.html uprint-lprm.1.html

# List of man pages in nroff format
ALL_TROFF=UPRINT.1.man uprint-remote.conf.5.man \
	uprint-lp.1.man uprint-lpstat.1.man uprint-cancel.1.man \
	uprint-lpr.1.man uprint-lpq.1.man uprint-lprm.1.man

#=== Build ==================================================================

all: $(ALL_HTML) $(ALL_TROFF)

#=== Install ================================================================

install: install-troff install-html

# Install the man pages in Nroff format
install-troff: $(ALL_TROFF)
	for d in 1 5;\
	  do \
	  for f in *.$$d.man;\
	    do \
	       echo "    man$$d/$$f"; \
	       ../$(INSTALLCP) $$f $(MANDIR)/man$$d/`basename $$f .man`; \
	    done;\
	  done
	../$(INSTALLLN) $(MANDIR)/man1/UPRINT.1 $(MANDIR)/man5/uprint.conf.5
	../$(INSTALLLN) $(MANDIR)/man1/UPRINT.1 $(MANDIR)/man8/uprint-newconf.8

# Install the man pages and other stuff in HTML format.
install-html: $(ALL_HTML)
	../$(MAKE_NEW_DIR) $(DOCSDIR)/uprint
	../$(INSTALLDATA) $(DOCSDIR)/uprint *.html

#=== Housekeeping ===========================================================

# Remove just editor backups.
clean:
	$(RMF) $(BACKUPS)

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) *.[158].html *.[158].man

# end of file
