#
# mouse:~ppr/src/samba/Makefile
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
# Last modified 8 May 2002.
#

include ../makeprogs/global.mk

#=== Inventory ==============================================================

PROGS=ppr2samba$(DOTEXE) \
	ppd2win95drv \
	ppd2windrv \
	samba_submitter \
	smb-protos.conf \
	sambaprint$(DOTEXE)

#=== Build ==================================================================

all: $(PROGS)

smb-protos.conf: smb-protos.conf.template
	$(RMF) smb-protos.conf
	sed -e 's!%HOMEDIR%!$(HOMEDIR)!g' -e 's!%VAR_SPOOL_PPR%!$(VAR_SPOOL_PPR)!g' \
		-e 's!%USER_PPR%!$(USER_PPR)!g' -e 's!%GROUP_PPR%!$(GROUP_PPR)!g' \
		-e 's!%CONFDIR%!$(CONFDIR)!g' \
		smb-protos.conf.template >smb-protos.conf
	chmod 444 smb-protos.conf

OBJS_PPR2SAMBA=ppr2samba.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
ppr2samba$(DOTEXE): $(OBJS_PPR2SAMBA)
	$(LD) $(LDFLAGS) -o $@ $(OBJS_PPR2SAMBA) $(INTLLIBS)

OBJS_SAMBAPRINT=sambaprint.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
sambaprint$(DOTEXE): $(OBJS_SAMBAPRINT)
	$(LD) $(LDFLAGS) -o $@ $(OBJS_SAMBAPRINT) $(INTLIBS) -ltdb

#=== Install ================================================================

install: $(PROGS) smb-protos.conf WIN95CP.BAT
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6711 $(HOMEDIR)/bin ppr2samba$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin ppd2win95drv
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib samba_submitter
	$(INSTALLCP) WIN95CP.BAT $(VAR_SPOOL_PPR)/drivers/WIN40/WIN95CP.BAT
	$(INSTALLCP) smb-protos.conf $(HOMEDIR)/lib/smb-protos.conf
	$(INSTALLPROGS) root $(GROUP_PPR) 4755 $(HOMEDIR)/lib sambaprint$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

# end of file

