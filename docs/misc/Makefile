#
# mouse:~ppr/src/docs/misc/Makefile
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
# Last modified 8 May 2001.
#

# Pull in settings from Configure.
include ../../makeprogs/global.mk
include ../../include/version.mk
include ../rules.mk

#=== Inventory ==============================================================

BUILT=\
	lpr-ext-notes.html

NOTBUILT=\
	index.html \
	ppr.lsm \
	announce.txt \
	LICENSE.txt \
	INSTALL.txt \
	TODO.txt \
	History.txt \
	PORTING.txt \
	docs_history.txt \
	docs_todo.txt \
	misc_dir_contents.txt \
	internationalization.txt \
	release_steps.html

#=== Build ==================================================================

all: $(NOTBUILT) $(BUILT)

#=== Install ================================================================

install: all
	../$(MAKE_NEW_DIR) $(DOCSDIR)/misc
	../$(INSTALLDATA) $(DOCSDIR)/misc $(NOTBUILT) $(BUILT)

#=== Housekeeping ===========================================================

clean:
	$(RMF) $(BACKUPS)

veryclean: clean
	$(RMF) $(BUILT)

# end of file

