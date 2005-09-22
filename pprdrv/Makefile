#
# mouse:~ppr/src/pprdrv/Makefile
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 18 April 2005.
#

include ../Makefile.conf

#=== Inventory ==============================================================

LIB_PROGS=pprdrv$(DOTEXE) ppr-gs$(DOTEXE)

LIB_DATA=lw-messages.conf pjl-messages.conf pserrors.conf

#=== Build ==================================================================

all: $(LIB_PROGS)

pprdrv$(DOTEXE): \
		pprdrv.o \
		pprdrv_interface.o \
		pprdrv_feedback.o \
		pprdrv_rip.o \
		pprdrv_flag.o \
		pprdrv_custom_hook.o \
		pprdrv_buf.o \
		pprdrv_ppd.o \
		pprdrv_res.o \
		pprdrv_capable.o \
		pprdrv_media.o \
		pprdrv_nup.o \
		pprdrv_req.o \
		pprdrv_signature.o \
		pprdrv_reason.o \
		pprdrv_patch.o \
		pprdrv_pagecount.o \
		pprdrv_commentary.o \
		pprdrv_ppop_status.o \
		pprdrv_progress.o \
		pprdrv_writemon.o \
		pprdrv_snmp.o \
		pprdrv_lw_messages.o pprdrv_pjl_messages.o \
		pprdrv_tt.o pprdrv_pfb.o pprdrv_mactt.o \
		pprdrv_notppd.o \
		pprdrv_persistent.o pprdrv_fault_debug.o \
		pprdrv_userparams.o \
		pprdrv_log.o \
		../libppr.a ../libgu.a ../libttf.a ../libpprdb.a
	$(LD) $(LDFLAGS) -o $@ $^ $(DBLIBS) $(SOCKLIBS) $(INTLLIBS) $(ZLIBLIBS)

ppr-gs$(DOTEXE): ppr-gs.o ../libgu.a ../libppr.a
	$(LD) $(LDFLAGS) -o $@ $^

#=== Install ================================================================

install: $(LIB_PROGS) $(LIB_DATA)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR) $(LIB_PROGS)
	$(INSTALLDATA) $(SHAREDIR)/misc $(LIB_DATA)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.o $(LIB_PROGS)

# end of file

