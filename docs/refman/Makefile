#
# mouse:~ppr/src/docs/refman/Makefile
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
# Last modified 1 August 2001.
#

# Pull in settings from Configure.
include ../../makeprogs/global.mk
include ../../include/version.mk
include ../rules.mk

#=== Inventory ==============================================================

# List of man pages in HTML format
ALL_HTML=ppr.1.html ppr-filters.1.html ppr2samba.8.html ppr-indexfonts.8.html ppop.1.html \
	ppad.8.html ppad-interfaces.8.html \
	pprd.8.html papsrv.8.html ppr-xfm.1.html \
	ppr-xgrant.1.html ppuser.8.html \
	lprsrv.8.html olprsrv.8.html

# List of man pages in nroff format
ALL_TROFF=ppr.1.man ppr-filters.1.man ppr2samba.8.man ppr-indexfonts.8.man ppop.1.man \
	ppad.8.man ppad-interfaces.8.man \
	pprd.8.man papsrv.8.man ppr-xfm.1.man \
	ppr-xgrant.1.man ppuser.8.man \
	lprsrv.8.man olprsrv.8.man


#=== Build ==================================================================

all-docs: $(ALL_HTML) $(ALL_TROFF)

#=== Install ================================================================

install: install-troff install-html

# Install the man pages in Nroff format
install-troff: $(ALL_TROFF)
	for d in 1 5 8;\
	  do \
	  ../$(MAKE_NEW_DIR) $(MANDIR)/man$$d; \
	  for f in *.$$d.man;\
	    do \
	       if [ $$f != "*.$$d.man" ];\
	         then \
	         echo "    man$$d/$$f"; \
	         ../$(INSTALLCP) $$f $(MANDIR)/man$$d/`basename $$f .man`; \
	         fi;\
	    done;\
	  done
	../$(INSTALLLN) $(MANDIR)/man8/ppr2samba.8 $(MANDIR)/man8/ppd2win95drv.8
	../$(INSTALLLN) $(MANDIR)/man8/lprsrv.8 $(MANDIR)/man5/lprsrv.conf.5
	../$(INSTALLLN) $(MANDIR)/man8/lprsrv.8 $(MANDIR)/man8/lprsrv-test.8

# Install the man pages and other stuff in HTML format.
install-html: $(ALL_HTML)
	../$(MAKE_NEW_DIR) $(DOCSDIR)/refman
	../$(INSTALLDATA) $(DOCSDIR)/refman *.html

#=== Housekeeping ===========================================================

# Remove just editor backups.
clean:
	$(RMF) $(BACKUPS)

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) *.[158].html *.[158].man

# end of file
