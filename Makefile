#
# mouse:~ppr/src/Makefile
# Copyright 1995--2001, Trinity College Computing Center.
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
# Last modified 13 June 2001.
#

#
# This is the master Makefile for the PPR spooling system.
#

# We have to pull this in because at some point we need to print the name of
# PPR's home directory.  (This file contains most of the system and local
# customization information.)
include makeprogs/global.mk

# The version number of PPR is found in this file and in version.h.
include include/version.mk

# Sourceforge CVS repository contact information.
CVS_RSH=ssh
CVSROOT=:ext:chappell@cvs.ppr.sourceforge.net:/cvsroot/ppr

# These are the subdirectories we will do make in:
SUBDIRS=\
	makeprogs \
	libgu \
	libppr \
	libscript \
	libpprdb \
	libttf \
	libuprint \
	procsets \
	pprd \
	pprdrv \
	fontutils \
	interfaces \
	responders \
	commentators \
	ppr \
	editps \
	ppop \
	ppad \
	ppuser \
	filter_lp \
	filter_dotmatrix \
	filters_misc \
	ppd \
	encodings \
	fonts \
	vendors \
	papsrv \
	lprsrv \
	samba \
	uprint \
	unixuser \
	printdesk \
	www \
	misc \
	po \
	fixup \
	init_and_cron \
	templates \
	docs

# These subdirectories are not ready for use yet.  We will only do
# "make clean" in them but not "make" or "make install".
SUBDIRS_CLEAN_ONLY=filter_pcl ipp tests

# This makes everything but doesn't install it.  This is so we can avoid
# disturbing an old version until we are sure the old one can be built.
all: symlinks
	@for i in $(SUBDIRS); \
		do \
		echo "==========================================="; \
		echo "=    running make in $$i"; \
		echo "==========================================="; \
		( cd $$i; make ) || exit 1; \
		echo; \
		done
	@echo
	@echo "Done making binaries.  You must now run \"make install\"."
	@echo

# Make the programs and install them.  It is allright to run this without
# doing "make all" first.
install: symlinks
	@for i in $(SUBDIRS); \
		do \
		echo "==========================================="; \
		echo "    running make install in $$i"; \
		echo "==========================================="; \
		( cd $$i; make install ) || exit 1; \
		echo; \
		done
	@echo
	@echo "Installation of binaries and support files done,"
	@echo "you must now run $(HOMEDIR)/fixup/fixup as root."
	@echo

# Update the .depend file in each directory.
depend:
	@for i in $(SUBDIRS) $(SUBDIRS_CLEAN_ONLY); \
		do \
		( cd $$i; make depend ) || exit 1; \
		done

# Remove editor backup files, object files, and executables
# from the source tree.
clean:
	@for i in $(SUBDIRS) $(SUBDIRS_CLEAN_ONLY); \
		do \
		echo "==========================================="; \
		echo "    running make clean in $$i"; \
		echo "==========================================="; \
		( cd $$i; make clean ) || exit 1; \
		echo; \
		done
	$(RMF) $(BACKUPS)
	( cd include && $(RMF) $(BACKUPS) )
	( cd nonppr && $(RMF) $(BACKUPS) )
	find . -name core -exec rm -f {} \; -print
	find . -name .tedfilepos -exec rm -f {} \; -print
	@echo
	@echo "All clean."
	@echo

# Just the documents, built before making the distribution.
docs:
	( cd docs && make )

# This one not only does what "make clean" does, it also restores
# "makeprogs/global.mk" to the unconfigured version.
distclean: clean
	cp makeprogs/global.mk.unconfigured makeprogs/global.mk
	@echo
	@echo "Configure undone."
	@echo

# This deletes difficult-to-generate files too.
veryclean: distclean
	( cd pprdrv; make veryclean )
	( cd papsrv; make veryclean )
	( cd docs; make veryclean )
	( cd www; make veryclean )
	@echo
	@echo "All generated files removed."
	@echo

# Pack the source up as an archive.  We build the install target to make
# absolutely sure that everything works and then to distclean so that
# we don't pack a lot of junk.
dist: docs distclean
	( cd po; ./extract_to_pot.sh )
	( cd /usr/local/src; tar cf - ppr-$(VERSION) | gzip --best >~ppr/ppr-$(VERSION).tar.gz )
	@echo
	@echo "Distribution archive built."
	@echo

# CVS doesn't preserve symlinks.  If INSTALL.txt is missing, run the hidden
# shell script in which they are preserved.
symlinks:
	if [ ! -f INSTALL.txt ]; then ./.restore_symlinks; fi

# CVS on Sourceforge.
cvs-import: veryclean
	./makeprogs/save_symlinks.sh
	#CVS_RSH=ssh cvs -d:ext:chappell@cvs.ppr.sourceforge.net:/cvsroot/ppr import ppr vendor start
	cvs import ppr vendor start

# end of file

