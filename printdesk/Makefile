#
# mouse:~ppr/src/printdesk/Makefile
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 22 September 2000.
#

include ../makeprogs/global.mk

PROGS=ppr-panel

ATALK_PROGS=nbp_lookup$(DOTEXE) ppr-chooser

PRINTDESK_SUB_SCRIPTS=ATchooser.pm BatchDialog.pm \
	GENfolder.pm GENjobbuttons.pm GENprnbuttons.pm \
	PPRlistqueue.pm PPRprintdialog.pm \
	PPRprnstatus.pm PPRstateupdate.pm \
	group.xbm printer.xbm

all: $(PROGS) $(ALLATALK)

allatalk: $(ATALK_PROGS)

install: $(PROGS) PrintDesk.pm $(PRINTDESK_SUB_SCRIPTS) $(INSTALLATALK)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(PROGS)
	$(INSTALLDATA) $(HOMEDIR)/lib PrintDesk.pm
	$(MAKE_NEW_DIR) $(HOMEDIR)/lib/PrintDesk
	$(INSTALLDATA) $(HOMEDIR)/lib/PrintDesk $(PRINTDESK_SUB_SCRIPTS)

installatalk: $(ATALK_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin ppr-chooser
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib nbp_lookup$(DOTEXE)

include .depend

nbp_lookup$(DOTEXE): nbp_lookup_$(ATALKTYPE).c
	$(CC) $(CFLAGS) -o nbp_lookup nbp_lookup_$(ATALKTYPE).c ../libgu.$(LIBEXT) $(ATALKLIBS)

clean:
	$(RMF) $(BACKUPS) $(OBJ) $(PROGS) $(ATALK_PROGS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file

