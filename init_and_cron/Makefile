#
# mouse:~ppr/src/init_and_cron/Makefile
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 22 November 2000.
#

include ../makeprogs/global.mk


PROGS_FIXUP=init_ppr
PROGS_LIB=cron_daily cron_hourly cron_run_daily$(DOTEXE)
PROGS_BIN=ppr-clean
PROGS=$(PROGS_FIXUP) $(PROGS_LIB) $(PROGS_BIN)

all: $(PROGS)

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/fixup $(PROGS_FIXUP)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib $(PROGS_LIB)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(PROGS_BIN)

cron_run_daily$(DOTEXE): cron_run_daily.$(OBJ)
	$(CC) $(CFLAGS) -o cron_run_daily$(DOTEXE) cron_run_daily.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)

depend:

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

# end of file
