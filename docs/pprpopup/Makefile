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
# Last modified 20 December 2001.
#

# Pull in settings from Configure.
include ../../makeprogs/global.mk
include ../../include/version.mk
include ../rules.mk

#=== Inventory ==============================================================

ALL_HTML=index.html

#=== Build ==================================================================

all-docs: 

#=== Install ================================================================

install: $(ALL_HTML)
	../$(MAKE_NEW_DIR) $(DOCSDIR)/pprpopup
	../$(INSTALLDATA) $(DOCSDIR)/pprpopup $(ALL_HTML)

#=== Housekeeping ===========================================================

# Remove just editor backups.
clean:
	$(RMF) $(BACKUPS)

# Remove anything that can be reconstructed.
veryclean: clean

# end of file
