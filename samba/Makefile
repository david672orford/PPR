#
# mouse:~ppr/src/samba/Makefile
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 19 December 2004.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=ppr2samba$(DOTEXE) \
	ppd2windrv \
	indexwindrivers \
	samba_submitter \
	smb-protos.conf

PROGS_TDB=sambaprint$(DOTEXE)

#=== Build ==================================================================

all: $(PROGS) $(ALL_TDB)

all-tdb: $(PROGS_TDB)

smb-protos.conf: smb-protos.conf.template
	$(RMF) smb-protos.conf
	sed -e 's!%HOMEDIR%!$(HOMEDIR)!g' -e 's!%VAR_SPOOL_PPR%!$(VAR_SPOOL_PPR)!g' \
		-e 's!%USER_PPR%!$(USER_PPR)!g' -e 's!%GROUP_PPR%!$(GROUP_PPR)!g' \
		-e 's!%CONFDIR%!$(CONFDIR)!g' \
		smb-protos.conf.template >smb-protos.conf
	chmod 444 smb-protos.conf

ppr2samba$(DOTEXE): ppr2samba.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS) $(ZLIBLIBS)

sambaprint$(DOTEXE): sambaprint.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS) -ltdb

#=== Install ================================================================

install: $(PROGS) smb-protos.conf $(INSTALL_TDB)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6711 $(HOMEDIR)/bin ppr2samba$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin ppd2windrv
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin indexwindrivers
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib samba_submitter
	$(INSTALLCP) smb-protos.conf $(HOMEDIR)/lib/smb-protos.conf

install-tdb: $(PROGS_TDB)
	$(INSTALLPROGS) root $(GROUP_PPR) 4755 $(HOMEDIR)/lib sambaprint$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS) $(PROGS_TDB)

# end of file

