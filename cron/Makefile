#
# mouse:~ppr/src/init_and_cron/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 6 March 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS_LIB=cron_daily cron_hourly cron_run_daily$(DOTEXE)
PROGS_BIN=ppr-clean
PROGS=$(PROGS_FIXUP) $(PROGS_LIB) $(PROGS_BIN)
USELIBS=../libgu.$(LIBEXT) ../libppr.$(LIBEXT) 

#=== Build ==================================================================

all: $(PROGS)

cron_run_daily$(DOTEXE): cron_run_daily.$(OBJ) $(USELIBS)
	$(LD) $(LDFLAGS) -o cron_run_daily$(DOTEXE) cron_run_daily.$(OBJ) $(USELIBS) $(INTLLIBS)

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib $(PROGS_LIB)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(PROGS_BIN)

#=== Housekeeping ===========================================================

depend:

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

# end of file
