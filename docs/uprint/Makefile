#
# mouse:~ppr/src/docs/uprint/Makefile
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 11 December 2004.
#

# Pull in settings from Configure.
include ../../Makefile.conf
include ../../makeprogs/paths.mk
include ../rules.mk

#=== Inventory ==============================================================

# List of man pages in HTML format
ALL_HTML=uprint-remote.conf.5.html \
	uprint-lp.1.html uprint-lpstat.1.html uprint-cancel.1.html \
	uprint-lpr.1.html uprint-lpq.1.html uprint-lprm.1.html uprint-lpc.8.html

# List of man pages in nroff format
ALL_TROFF=uprint-remote.conf.5.man \
	uprint-lp.1.man uprint-lpstat.1.man uprint-cancel.1.man \
	uprint-lpr.1.man uprint-lpq.1.man uprint-lprm.1.man uprint-lpc.8.man

# List of PODs for cross references
LIBPODS=uprint-cancel.1,uprint-lp.1,uprint-lpq.1,uprint-lpr.1,uprint-lprm.1,uprint-lpc.8,uprint-lpstat.1,uprint-remote.conf.5,lprsrv.8

#=== Build ==================================================================

all-docs: $(ALL_HTML) $(ALL_TROFF)

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

# Install the man pages and other stuff in HTML format.
install-html: $(ALL_HTML)
	../$(MAKE_NEW_DIR) $(DOCSDIR)/uprint
	../$(INSTALLDATA) $(DOCSDIR)/uprint *.html

#=== Housekeeping ===========================================================

# Remove just editor backups.
clean:
	$(RMF) $(BACKUPS) *.tmp

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) *.[158].html *.[158].man

# end of file
