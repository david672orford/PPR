#
# mouse:~ppr/src/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 7 December 2006.
#

#
# This is the master Makefile for the PPR spooling system.
#

# Sign to Configure
export CONFIGURE_RUNNING_FROM_MAKEFILE=1

# We have to pull this in because at some point we need to print the name of
# PPR's home directory.  (This file contains most of the system and local
# customization information.)
include Makefile.conf

# We pull in this because we need SHORT_VERSION.
include makeprogs/version.mk

#=== Inventory ==============================================================

# These are the subdirectories we will do make in:
SUBDIRS=\
	z_install_begin \
	makeprogs \
	libgu \
	libppr \
	ppr-tclsh \
	libscript \
	libpprdb \
	libttf \
	procsets \
	pprd \
	pprdrv \
	fontutils \
	interfaces \
	browsers \
	responders \
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
	nonppr_vendors \
	papd \
	lprsrv \
	samba \
	ipp \
	unixuser \
	printdesk \
	www \
	misc \
	po \
	cron \
	templates \
	docs \
	z_install_end

# These subdirectories are not ready for use yet.  We will only do
# "make clean" in them but not "make" or "make install".
SUBDIRS_CLEAN_ONLY=filter_pcl filters_typeset tests

#=== Build ==================================================================

all: symlinks-restore config.h
	@for i in $(SUBDIRS); \
		do \
		echo "==========================================="; \
		echo "=    running $(MAKE) in $$i"; \
		echo "==========================================="; \
		( cd $$i && $(MAKE) ) || exit 1; \
		echo; \
		done
	@echo
	@echo "Done making binaries.  You must now run \"$(MAKE) install\"."
	@echo

#=== Install ================================================================

install: symlinks-restore config.h
	@for i in $(SUBDIRS); \
		do \
		echo "==========================================="; \
		echo "    running $(MAKE) install in $$i"; \
		echo "==========================================="; \
		( cd $$i && $(MAKE) install ) || exit 1; \
		echo; \
		done

#=== Configure  =============================================================

# The script Configure calls this in order to pick up the old settings 
# exported from Makefile.conf.
configure:
	echo "Running \"$$shell ./Configure\"..."
	./Configure

config.h: config.h.in Makefile.conf subst_tool$(DOTEXE)
	./subst_tool <config.h.in >config.h	

subst_tool$(DOTEXE): subst_tool.c
	$(CC) $(CFLAGS) -o $@ $^

#=== Distribution Archive Generation =======================================

# Just the documents, built before making the distribution .tar.gz file.
dist-docs:
	@echo "==========================================="
	@echo "    Building Documentation"
	@echo "==========================================="
	( cd docs && $(MAKE) MAKEFLAGS= all-docs ) || exit 1
	@echo

# Make sure the hard-to-build docs are built, remove the easy to build stuff,
# and build the tar file.
dist: dist-docs clean unconfigure
	( cd po; make pot )
	( cd $$HOME; tar -zcf ppr-$(SHORT_VERSION).tar.gz --exclude CVS ppr-$(SHORT_VERSION) )
	@echo
	@echo "Distribution archive built."
	@echo

#=== CVS ====================================================================

# CVS doesn't preserve symlinks.  If INSTALL.txt is missing, run the hidden
# shell script in which they are preserved.
symlinks-restore:
	if [ -d CVS ]; then ./.restore_symlinks; fi

# This creates the file that the symbolic links are restored from.
# We must do a clean first since the build process creates a few 
# additional symbolic links to executables.
symlinks-save: clean
	./makeprogs/save_symlinks.sh

#=== Housekeeping ===========================================================

ctags:
	ctags -R

# Update the .depend file in each directory.
depend:
	@for i in $(SUBDIRS) $(SUBDIRS_CLEAN_ONLY); \
		do \
		echo "==========================================="; \
		echo "    running $(MAKE) depend in $$i"; \
		echo "==========================================="; \
		( cd $$i && $(MAKE) depend ) || exit 1; \
		done

# Remove editor backup files, object files, and executables
# from the source tree.
clean:
	@for i in $(SUBDIRS) $(SUBDIRS_CLEAN_ONLY); \
		do \
		echo "==========================================="; \
		echo "    running $(MAKE) clean in $$i"; \
		echo "==========================================="; \
		( cd $$i && $(MAKE) clean ) || exit 1; \
		echo; \
		done
	$(RMF) $(BACKUPS)
	$(RMF) *-stamp		# debian package builder
	$(RMF) root.sh
	$(RMF) subst_tool$(DOTEXE)
	( cd include && $(RMF) $(BACKUPS) )
	( cd nonppr_misc && $(RMF) $(BACKUPS) )
	find . -name core -exec rm -f {} \; -print
	find . -name .tedfilepos -exec rm -f {} \; -print
	$(RMF) tags
	@echo
	@echo "All clean."
	@echo

# This deletes difficult-to-generate files too.
veryclean: clean unconfigure
	( cd po; make veryclean )
	( cd ppad; make veryclean )
	( cd www; make veryclean )
	( cd docs; make veryclean )
	@echo
	@echo "All generated files removed."
	@echo

unconfigure:
	cp makeprogs/Makefile.conf.unconfigured Makefile.conf
	cp makeprogs/config.h.unconfigured config.h

# end of file

