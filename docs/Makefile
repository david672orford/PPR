#
# mouse:~ppr/src/docs/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 18 April 2006.
#

# Pull in settings from Configure.
include ../Makefile.conf
include ../makeprogs/paths.mk
include ./rules.mk

#=== Inventory ==============================================================

PROGS=ppdoc
HTML=index.html external_reference.html

SUBDIRS=refman pprinst pprhack misc

#=== Build ==================================================================

all: $(PROGS) all-docs

all-docs: $(HTML)
	@for i in $(SUBDIRS); do echo "=== $$i ==="; ( cd $$i && $(MAKE) all-docs ); done

#=== Install ================================================================

install: $(PROGS) $(HTML)
	$(MAKE_NEW_DIR) $(DOCSDIR)
	$(INSTALLDATA) $(DOCSDIR) $(HTML)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(BINDIR) $(PROGS)
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) install ); done

#=== Housekeeping ===========================================================

depend:

clean:
	$(RMF) $(BACKUPS) $(PROGS)
	@for i in $(SUBDIRS); do echo "=== $$i ==="; ( cd $$i && $(MAKE) clean ); done

veryclean:
	$(RMF) $(BACKUPS) $(PROGS)
	@for i in $(SUBDIRS); do echo "=== $$i ==="; ( cd $$i && $(MAKE) veryclean ); done

# end of file

