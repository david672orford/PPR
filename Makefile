#
# mouse:~ppr/src/Makefile
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
# Last modified 31 October 2003.
#

#
# This is the master Makefile for the PPR spooling system.
#

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
	nonppr_tcl \
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
	nonppr_vendors \
	papsrv \
	lprsrv \
	samba \
	pprpopup \
	uprint \
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
SUBDIRS_CLEAN_ONLY=filter_pcl filters_typeset ipp tests papd browsers

#=== Build ==================================================================

all: symlinks-restore
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

install: symlinks-restore
	@for i in $(SUBDIRS); \
		do \
		echo "==========================================="; \
		echo "    running $(MAKE) install in $$i"; \
		echo "==========================================="; \
		( cd $$i && $(MAKE) install ) || exit 1; \
		echo; \
		done

#=== Distribution Archive Generation =======================================

# Just the documents, built before making the distribution.
dist-docs:
	@echo "==========================================="
	@echo "    Building Documentation"
	@echo "==========================================="
	( cd docs && $(MAKE) MAKEFLAGS= all-docs ) || exit 1
	@echo

# Make sure the hard-to-build docs are built, remove the easy to build stuff,
# and build the tar file.
dist: dist-docs clean unconfigure
	( cd po; ./extract_to_pot.sh )
	( cd /usr/local/src; tar zcf $$HOME/ppr-$(SHORT_VERSION).tar.gz ppr-$(SHORT_VERSION) --exclude 'docbook-*-*' --exclude CVS )
	@echo
	@echo "Distribution archive built."
	@echo

#=== CVS ====================================================================

# CVS doesn't preserve symlinks.  If INSTALL.txt is missing, run the hidden
# shell script in which they are preserved.
symlinks-restore:
	if [ ! -f libgu/gu_md5.c ]; then bash ./.restore_symlinks; fi

# This creates the file that the symbolic links are restored from.
symlinks-save:
	./makeprogs/save_symlinks.sh

#=== Housekeeping ===========================================================

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
	$(RMF) root.sh
	( cd include && $(RMF) $(BACKUPS) )
	( cd nonppr_misc && $(RMF) $(BACKUPS) )
	find . -name core -exec rm -f {} \; -print
	find . -name .tedfilepos -exec rm -f {} \; -print
	@echo
	@echo "All clean."
	@echo

# This deletes difficult-to-generate files too.
veryclean: clean unconfigure
	( cd papsrv; make veryclean )
	( cd docs; make veryclean )
	( cd www; make veryclean )
	@echo
	@echo "All generated files removed."
	@echo

unconfigure:
	cp makeprogs/Makefile.conf.unconfigured Makefile.conf

# end of file

