#
# mouse:~ppr/src/docs/pprinst/Makefile
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
# Last modified 21 April 2004.
#

# Pull in settings from Configure, the PPR version number, and special rules
# for building documentation.
include ../../Makefile.conf
include ../../makeprogs/paths.mk
include ../rules.mk

#=== Inventory ==============================================================

BUILT=pprinst.html pprinst.pdf

NONBUILT=web_start.png \
	web_control.png \
	web_jobs.png \
	web_prn_control.png \
	web_prn_addwiz.png \
	web_prn_properties1.png \
	perltk_control.png \
	perltk_prn_status.png \
	perltk_queue.png \
	perltk_submit.png

#=== Build ==================================================================

all-docs: $(BUILT) $(NONBUILT)

pprinst.sgml: pprinst_adding_autobin.sgml \
		pprinst_adding_groups.sgml \
		pprinst_adding_lmx.sgml \
		pprinst_adding_lprsrv.sgml \
		pprinst_adding_papd.sgml \
		pprinst_adding_samba.sgml \
		pprinst_adding_spooler.sgml \
		pprinst_backup.sgml \
		pprinst_queue.sgml \
		pprinst_installation.sgml \
		pprinst_job_submit.sgml \
		pprinst_operators_duties.sgml \
		pprinst_removing.sgml \
		pprinst_replacing_printer.sgml \
		pprinst_start_restart.sgml \
		pprinst_user_notification.sgml \
		pprinst_user_interfaces.sgml \
		../shared/legal.sgml
	touch pprinst.sgml

#=== Validity Check ================================================================

lint:
	SGML_CATALOG_FILES=$(SGML_CATALOG_FILES) $(XMLLINT) --catalogs pprinst.sgml

#=== Install ================================================================

install: all-docs
	../$(MAKE_NEW_DIR) $(DOCSDIR)/pprinst
	../$(INSTALLDATA) $(DOCSDIR)/pprinst $(BUILT) $(NONBUILT)

#=== Housekeeping ===========================================================

# Remove editor backup files and TeX intermediate files but not the built
# documentation.
clean:
	$(RMF) $(BACKUPS)
	$(RMF) *.dvi *.aux *.log *.toc *.tex *.ps

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) $(BUILT)

# end of file
