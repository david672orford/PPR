#
# mouse:~ppr/src/docs/refman/Makefile
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
# Last modified 2 August 2003.
#

# Pull in settings from Configure.
include ../../Makefile.conf
include ../../makeprogs/paths.mk
include ../rules.mk

#=== Inventory ==============================================================

# List of man pages in HTML format
ALL_HTML=ppdoc.1.html \
	ppr.1.html \
	ppr-filters.1.html \
	ppr2samba.8.html \
	ppd2macosdrv.8.html \
	ppr-index.8.html \
	ppop.1.html \
	ppad.8.html \
	ppad-interfaces.8.html \
	pprd.8.html \
	papsrv.8.html \
	papd.8.html \
	ppr-xfm.1.html \
	ppr-xgrant.1.html \
	ppuser.8.html \
	lprsrv.8.html \
	olprsrv.8.html \
	ppr-hexdump.1.html \
	ppr-config.8.html \
	ppr-passwd.1.html \
	ppr-clean.8.html \
	ppr-testpage.1.html \
	ppr-ttf2ps.1.html \
	ppr-web.1.html \
	ppr-panel.1.html \
	ppr-followme.1.html \
	ppr-popup.1.html

# List of man pages in nroff format
ALL_TROFF=ppdoc.1.man \
	ppr.1.man \
	ppr-filters.1.man \
	ppr2samba.8.man \
	ppd2macosdrv.8.man \
	ppr-index.8.man \
	ppop.1.man \
	ppad.8.man \
	ppad-interfaces.8.man \
	pprd.8.man \
	papsrv.8.man \
	papd.8.man \
	ppr-xfm.1.man \
	ppr-xgrant.1.man \
	ppuser.8.man \
	lprsrv.8.man \
	olprsrv.8.man \
	ppr-hexdump.1.man \
	ppr-config.8.man \
	ppr-passwd.1.man \
	ppr-clean.8.man \
	ppr-testpage.1.man \
	ppr-ttf2ps.1.man \
	ppr-web.1.man \
	ppr-panel.1.man \
	ppr-followme.1.man \
	ppr-popup.1.man

# List of PODs for cross references
LIBPODS=ppdoc.1,ppr.1,ppr-filters.1,ppr2samba.8,ppd2macosdrv.8,ppr-index.8,ppop,ppad.8,ppad-interfaces.8,pprd.8,papsrv.8,papd.8,ppr-xfm.1,ppr-xgrant.1,ppuser.8,lprsrv.8,olprsrv.8,ppr-hexdump.1,ppr-config.8,ppr-passwd.1,ppr-clean.1,ppr-testpage.1,ppr-ttf2ps.1,ppr-web.1,ppr-panel.1,ppr-followme.1,ppr-popup.1

#=== Build ==================================================================

all-docs: $(ALL_HTML) $(ALL_TROFF)

#=== Install ================================================================

install: install-troff install-html

# Install the man pages in Nroff format.
# Note that this removes the UPRINT man pages too.
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
	../$(INSTALLLN) ppr2samba.8 $(MANDIR)/man8/ppd2win95drv.8
	../$(INSTALLLN) ../man8/lprsrv.8 $(MANDIR)/man5/lprsrv.conf.5
	../$(INSTALLLN) ../man8/lprsrv.8 $(MANDIR)/man8/lprsrv-test.8

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
