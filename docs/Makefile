#
# mouse:~ppr/src/docs/Makefile
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
# Last modified 21 December 2001.
#

# Pull in settings from Configure.
include ../makeprogs/global.mk
include ../include/version.mk
include ./rules.mk

#=== Inventory ==============================================================

PROGS=ppdoc
HTML=index.html

SUBDIRS=refman uprint pprdoc pprinst pprhack misc pprpopup

#=== Build ==================================================================

all: $(PROGS) all-docs

all-docs: $(HTML)
	@for i in $(SUBDIRS); do echo "=== $$i ==="; ( cd $$i && $(MAKE) all-docs ); done

#=== Install ================================================================

install: $(PROGS) $(HTML)
	$(MAKE_NEW_DIR) $(DOCSDIR)
	$(INSTALLDATA) $(DOCSDIR) $(HTML)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(PROGS)
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) install ); done

#=== Housekeeping ===========================================================

depend:

clean:
	$(RMF) $(BACKUPS) $(PROGS)
	@for i in $(SUBDIRS); do echo "=== $$i ==="; ( cd $$i && make clean ); done

veryclean:
	$(RMF) $(BACKUPS) $(PROGS)
	@for i in $(SUBDIRS); do echo "=== $$i ==="; ( cd $$i && make veryclean ); done

# end of file

