#
# mouse:~ppr/src/docs/pprinst/Makefile
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
# Last modified 16 January 2003.
#

# Pull in settings from Configure, the PPR version number, and special rules
# for building documentation.
include ../../Makefile.conf
include ../../include/version.mk
include ../rules.mk

#=== Inventory ==============================================================

BUILT=pprinst.html pprinst.ps pprinst.pdf

#=== Build ==================================================================

all-docs: $(BUILT)

pprinst.sgml: pprinst_adding_autobin.sgml \
		pprinst_adding_groups.sgml \
		pprinst_adding_lmx.sgml \
		pprinst_adding_lprsrv.sgml \
		pprinst_adding_papsrv.sgml \
		pprinst_adding_samba.sgml \
		pprinst_adding_spooler.sgml \
		pprinst_backup.sgml \
		pprinst_canceling_jobs.sgml \
		pprinst_installation.sgml \
		pprinst_job_submit.sgml \
		pprinst_operators_duties.sgml \
		pprinst_removing.sgml \
		pprinst_replacing_printer.sgml \
		pprinst_start_restart.sgml \
		pprinst_user_notification.sgml \
		pprinst_viewing_queue.sgml
	touch pprinst.sgml

#=== Install ================================================================

install: all-docs
	../$(MAKE_NEW_DIR) $(DOCSDIR)/pprinst
	../$(INSTALLDATA) $(DOCSDIR)/pprinst $(BUILT)

#=== Housekeeping ===========================================================

# Remove editor backup files and TeX intermediate files but not the built
# documentation.
clean:
	$(RMF) $(BACKUPS)
	$(RMF) *.dvi *.aux *.log *.toc *.tex

# Remove anything that can be reconstructed.
veryclean: clean
	$(RMF) $(BUILT)

# end of file

