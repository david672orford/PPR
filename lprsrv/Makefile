#
# mouse:~ppr/src/lprsrv/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
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
# Last modified 10 April 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

LIB_PROGS=lprsrv$(DOTEXE)
BIN_PROGS=lprsrv-test$(DOTEXE)
PROGS=$(BIN_PROGS) $(LIB_PROGS)

USELIBS=../libgu.a ../libppr.a 

#=== Build ==================================================================

all: $(PROGS)

lprsrv$(DOTEXE): \
		lprsrv.o lprsrv_client_info.o lprsrv_conf.o \
		lprsrv_print.o lprsrv_list.o lprsrv_cancel.o \
		uprint_claim_ppr.o \
		uprint_obj.o \
		uprint_sysv.o \
		uprint_xlate.o \
		uprint_print_ppr.o \
		uprint_run.o \
		$(USELIBS)
	$(LD) $(LDFLAGS) -o $@ $^ $(SOCKLIBS) $(INTLLIBS)

lprsrv-test$(DOTEXE): lprsrv-test.o lprsrv_conf.o $(USELIBS)
	$(LD) $(LDFLAGS) -o $@ $^ $(SOCKLIBS) $(INTLLIBS)

#=== Install ================================================================

install: $(PROGS) lprsrv.conf.sample
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR) $(LIB_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(BINDIR) $(BIN_PROGS)
	$(INSTALLCP) lprsrv.conf.sample $(CONFDIR)/lprsrv.conf.sample

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) $(BACKUPS) *.o $(PROGS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file

