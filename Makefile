#
# mouse:~ppr/src/Makefile
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 8 May 2001.
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
all:
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
install:
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

# Pack the source up as an archive.  You must do a "make clean"
# before this if you don't want to pack up a lot of junk.
dist: distclean
	( cd po; ./extract_to_pot.sh )
	( cd /usr/local/src; tar cf - ppr-$(VERSION) | gzip --best >~ppr/ppr-$(VERSION).tar.gz )
	@echo
	@echo "Distribition archive built."
	@echo

# Check into CVS on Sourceforge.
cvs: veryclean
	CVS_RSH=ssh cvs -d:ext:chappell@cvs.ppr.sourceforge.net:/cvsroot/ppr import ppr vendor start

# end of file

