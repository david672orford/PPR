#
# mouse:~ppr/src/unixuser/Makefile
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
# Last modified 27 November 2002.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BIN_PROGS_SUID=ppr-xgrant$(DOTEXE) ppr-hexdump$(DOTEXE)
BIN_PROGS=ppr-xfm
LIB_PROGS=
FIXUP_SCRIPTS=login_ppr.sh login_ppr.csh
PROGS=$(BIN_PROGS) $(BIN_PROGS_SUID) $(LIB_PROGS)

#=== Build ==================================================================

all: $(PROGS)

# Explicit rule because of System V release 4.0 make bugs.
ppr-xgrant$(DOTEXE): ppr-xgrant.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(CC) $(CFLAGS) -o ppr-xgrant$(DOTEXE) ppr-xgrant.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) root        $(GROUP_PPR) 6755 $(HOMEDIR)/bin $(BIN_PROGS_SUID)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755  $(HOMEDIR)/bin $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755  $(HOMEDIR)/lib $(LIB_PROGS) 
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 644  $(HOMEDIR)/fixup $(FIXUP_SCRIPTS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS)

# end of file

