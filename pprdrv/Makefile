#
# mouse:~ppr/src/pprdrv/Makefile
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

include ../Makefile.conf

#=== Inventory ==============================================================

LIB_PROGS=pprdrv$(DOTEXE) ppr-gs$(DOTEXE)

LIB_DATA=lw-messages.conf pjl-messages.conf pserrors.conf

#=== Build ==================================================================

all: $(LIB_PROGS)

OBJS1=pprdrv.$(OBJ) \
	pprdrv_interface.$(OBJ) \
	pprdrv_feedback.$(OBJ) \
	pprdrv_rip.$(OBJ) \
	pprdrv_flag.$(OBJ) \
	pprdrv_custom_hook.$(OBJ) \
	pprdrv_buf.$(OBJ) \
	pprdrv_ppd.$(OBJ) \
	pprdrv_res.$(OBJ) \
	pprdrv_capable.$(OBJ) \
	pprdrv_media.$(OBJ) \
	pprdrv_nup.$(OBJ) \
	pprdrv_req.$(OBJ) \
	pprdrv_signature.$(OBJ) \
	pprdrv_reason.$(OBJ) \
	pprdrv_patch.$(OBJ) \
	pprdrv_pagecount.$(OBJ) \
	pprdrv_commentary.$(OBJ) \
	pprdrv_ppop_status.$(OBJ) \
	pprdrv_progress.$(OBJ) \
	pprdrv_writemon.$(OBJ) \
	pprdrv_snmp.$(OBJ) \
	pprdrv_lw_messages.$(OBJ) pprdrv_pjl_messages.$(OBJ) pprdrv_snmp_messages.$(OBJ) \
	pprdrv_tt.$(OBJ) pprdrv_pfb.$(OBJ) pprdrv_mactt.$(OBJ) \
	pprdrv_notppd.$(OBJ) \
	pprdrv_persistent.$(OBJ) pprdrv_fault_debug.$(OBJ) \
	pprdrv_userparams.$(OBJ) \
	pprdrv_log.$(OBJ) \
	../libppr.$(LIBEXT) ../libgu.$(LIBEXT) ../libttf.$(LIBEXT) ../libpprdb.$(LIBEXT)

pprdrv$(DOTEXE): $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(DBLIBS) $(SOCKLIBS) $(INTLLIBS) $(ZLIBLIBS)

OBJS2=ppr-gs.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
ppr-gs$(DOTEXE): $(OBJS2)
	$(LD) $(LDFLAGS) -o $@ $(OBJS2)

#=== Install ================================================================

install: $(LIB_PROGS) $(LIB_DATA)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib $(LIB_PROGS)
	$(INSTALLDATA) $(SHAREDIR)/lib $(LIB_DATA)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(LIB_PROGS)

# end of file

