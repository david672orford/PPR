#
# mouse:~ppr/src/libppr/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 17 May 2002.
#

include ../Makefile.conf

LIBOBJS=\
	int_prpapst.$(OBJ) int_cmdline.$(OBJ) \
	int_acache.$(OBJ) int_debug.$(OBJ) int_exit.$(OBJ) \
	int_con_tcpip.$(OBJ) int_pe_tcpip.$(OBJ) \
	int_snmp.$(OBJ) \
	int_copy_job.$(OBJ) \
	datestamp.$(OBJ) \
	tokenize.$(OBJ) \
	readqfile.$(OBJ) writeqfile.$(OBJ) unreadqfile.$(OBJ) parse_qfname.$(OBJ) \
	quote.$(OBJ) \
	findres.$(OBJ) \
	protected.$(OBJ) money.$(OBJ) \
	charge.$(OBJ) jobid.$(OBJ) pagesize.$(OBJ) \
	options.$(OBJ) \
	dimens.$(OBJ) foptions.$(OBJ) ali_str.$(OBJ) \
	ppr_gcmd.$(OBJ) gnodename.$(OBJ) readppd.$(OBJ) \
	destspec.$(OBJ) unsafe.$(OBJ) prune_env.$(OBJ) \
	acl.$(OBJ) \
	cs2enc.$(OBJ) enc2font.$(OBJ) font.$(OBJ) \
	ppr_fnamef.$(OBJ) \
	interfaces.$(OBJ) \
	alert.$(OBJ) \
	tail_status.$(OBJ) \
	query.$(OBJ) \
	queueinfo.$(OBJ) \
	pagemask.$(OBJ)

TARGETS=../libppr.$(LIBEXT)

all: $(TARGETS)

install: all

include .depend

# This build rule for this module is listed explicitly because of the
# -D option on in the cc command.
ali_str.$(OBJ): ali_str.c
	$(CC) $(CFLAGS) -DATALKTYPE_$(ATALKTYPE) -c ali_str.c

../libppr.$(LIBEXT): $(LIBOBJS)
	$(RMF) ../libppr.$(LIBEXT)
	#$(LIBCMD) ../libppr.$(LIBEXT) >/dev/null 2>&1
	$(LIBCMD) ../libppr.$(LIBEXT) $(LIBOBJS)

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(TARGETS)

# end of file

