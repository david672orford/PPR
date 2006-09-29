#
# mouse:~ppr/src/samba/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 29 September 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=ppr2samba$(DOTEXE) \
	samba_submitter \
	smb-protos.conf

PROGS_TDB=indexwindrivers sambaprint$(DOTEXE)

#=== Build ==================================================================

all: $(PROGS) $(ALL_TDB)

all-tdb: $(PROGS_TDB)

#smb-protos.conf: smb-protos.conf.template
#	$(RMF) smb-protos.conf
#	../subst_tool <smb-protos.conf.template >smb-protos.conf
#	chmod 444 smb-protos.conf

ppr2samba$(DOTEXE): ppr2samba.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS) $(ZLIBLIBS)

sambaprint$(DOTEXE): sambaprint.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS) -ltdb

#=== Install ================================================================

install: $(PROGS) smb-protos.conf $(INSTALL_TDB)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6711 $(BINDIR) ppr2samba$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755  $(LIBDIR) samba_submitter
	$(INSTALLCP) smb-protos.conf $(LIBDIR)/smb-protos.conf

install-tdb: $(PROGS_TDB)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR)  755 $(LIBDIR) indexwindrivers
	$(INSTALLPROGS) root        $(GROUP_PPR) 4755 $(LIBDIR) sambaprint$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.o $(BACKUPS) $(PROGS) $(PROGS_TDB)

# end of file

