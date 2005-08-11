#
# mouse:~ppr/src/lprsrv/Makefile
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
# Last modified 9 August 2005.
#

include ../Makefile.conf

#=== Inventory ==============================================================

LIB_PROGS=lprsrv$(DOTEXE)
BIN_PROGS=lprsrv-test$(DOTEXE)
PROGS=$(BIN_PROGS) $(LIB_PROGS)

USELIBS=../libgu.$(LIBEXT) ../libppr.$(LIBEXT) 

#=== Build ==================================================================

all: $(PROGS)

lprsrv$(DOTEXE): \
		lprsrv.$(OBJ) lprsrv_client_info.$(OBJ) lprsrv_conf.$(OBJ) \
		lprsrv_print.$(OBJ) lprsrv_list.$(OBJ) lprsrv_cancel.$(OBJ) \
		lprsrv_standalone.$(OBJ) \
		uprint_claim_ppr.$(OBJ) \
		uprint_obj.$(OBJ) \
		uprint_sysv.$(OBJ) \
		uprint_xlate.$(OBJ) \
		uprint_print_ppr.$(OBJ) \
		uprint_run.$(OBJ) \
		$(USELIBS)
	$(LD) $(LDFLAGS) -o $@ $^ $(SOCKLIBS) $(INTLLIBS)

lprsrv-test$(DOTEXE): lprsrv-test.$(OBJ) lprsrv_conf.$(OBJ) $(USELIBS)
	$(LD) $(LDFLAGS) -o $@ $^ $(SOCKLIBS) $(INTLLIBS)

#=== Install ================================================================

install: $(PROGS) lprsrv.conf.sample
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR) $(LIB_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(BINDIR) $(BIN_PROGS)
	$(INSTALLCP) lprsrv.conf.sample $(CONFDIR)/lprsrv.conf.sample

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file

