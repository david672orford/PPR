#
# mouse:~ppr/src/docs/Makefile
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 8 August 2005.
#

# Pull in settings from Configure.
include ../Makefile.conf
include ../makeprogs/paths.mk
include ./rules.mk

#=== Inventory ==============================================================

PROGS=ppdoc
HTML=index.html

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

