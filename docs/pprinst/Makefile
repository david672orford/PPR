#
# mouse:~ppr/src/docs/pprinst/Makefile
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
# Last modified 1 August 2001.
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

