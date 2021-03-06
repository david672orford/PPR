#
# mouse:~ppr/src/docs/misc/Makefile
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
# Last modified 25 October 2004.
#

# Pull in settings from Configure.
include ../../Makefile.conf
include ../../makeprogs/paths.mk
include ../rules.mk

#=== Inventory ==============================================================

BUILT=\
	lpr-ext-notes.html

NOTBUILT=\
	index.html \
	announce.txt \
	LICENSE.txt \
	INSTALL.txt \
	PORTING.txt \
	main_history.txt \
	docs_history.txt \
	misc_dir_contents.txt \
	internationalization.txt \
	release_steps.html 

#=== Build ==================================================================

all-docs: $(NOTBUILT) $(BUILT)

#=== Validity Check ================================================================

lint:
	SGML_CATALOG_FILES=$(SGML_CATALOG_FILES) $(XMLLINT) --catalogs lpr-ext-notes.sgml

#=== Install ================================================================

install: all-docs
	../$(MAKE_NEW_DIR) $(DOCSDIR)/misc
	../$(INSTALLDATA) $(DOCSDIR)/misc $(NOTBUILT) $(BUILT)

#=== Housekeeping ===========================================================

clean:
	$(RMF) $(BACKUPS)

veryclean: clean
	$(RMF) $(BUILT)

# end of file

