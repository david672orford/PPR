#
# mouse:~ppr/src/pprdrv/Makefile
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

include ../Makefile.conf

#=== Inventory ==============================================================

BIN_PROGS=pprdrv$(DOTEXE)

LIB_DATA=lw-messages.conf pjl-messages.conf pserrors.conf

#=== Build ==================================================================

all: $(BIN_PROGS)

OBJS1=pprdrv.$(OBJ) pprdrv_interface.$(OBJ) pprdrv_feedback.$(OBJ) \
	pprdrv_rip.$(OBJ) \
	pprdrv_flag.$(OBJ) pprdrv_custom_hook.$(OBJ) \
	pprdrv_buf.$(OBJ) pprdrv_ppd.$(OBJ) pprdrv_ppd_parse.$(OBJ) \
	pprdrv_res.$(OBJ) pprdrv_capable.$(OBJ) pprdrv_media.$(OBJ) \
	pprdrv_nup.$(OBJ) pprdrv_req.$(OBJ) pprdrv_signature.$(OBJ) \
	pprdrv_reason.$(OBJ) pprdrv_patch.$(OBJ) \
	pprdrv_pagecount.$(OBJ) \
	pprdrv_commentary.$(OBJ) pprdrv_ppop_status.$(OBJ) \
	pprdrv_progress.$(OBJ) pprdrv_writemon.$(OBJ) \
	pprdrv_snmp.$(OBJ) \
	pprdrv_lw_messages.$(OBJ) pprdrv_pjl_messages.$(OBJ) pprdrv_snmp_messages.$(OBJ) \
	pprdrv_tt.$(OBJ) pprdrv_pfb.$(OBJ) pprdrv_mactt.$(OBJ) \
	pprdrv_notppd.$(OBJ) \
	pprdrv_persistent.$(OBJ) pprdrv_fault_debug.$(OBJ) \
	pprdrv_userparams.$(OBJ) \
	../libppr.$(LIBEXT) ../libgu.$(LIBEXT) ../libttf.$(LIBEXT) ../libpprdb.$(LIBEXT)
pprdrv$(DOTEXE): $(OBJS1) 
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(DBLIBS) $(SOCKLIBS) $(INTLLIBS)

# How to build the C source for the PPD file lexer:
pprdrv_ppd_parse.c: pprdrv_ppd_parse.l
	$(LEX) -v pprdrv_ppd_parse.l
	$(MV) lex.yy.c pprdrv_ppd_parse.c

#=== Install ================================================================

install: $(BIN_PROGS) $(LIB_DATA)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib $(BIN_PROGS)
	$(INSTALLDATA) $(SHAREDIR)/lib $(LIB_DATA)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(BIN_PROGS)

veryclean: clean
	$(RMF) pprdrv_ppd_parse.c

# end of file

