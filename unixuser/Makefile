#
# mouse:~ppr/src/unixuser/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 7 January 2002.
#

include ../Makefile.conf

BIN_PROGS_SUID=ppr-xgrant$(DOTEXE)
BIN_PROGS=ppr-xfm
LIB_PROGS=
FIXUP_SCRIPTS=login_ppr.sh login_ppr.csh
PROGS=$(BIN_PROGS) $(BIN_PROGS_SUID) $(LIB_PROGS)

all: $(PROGS)

install: $(PROGS)
	$(INSTALLPROGS) root        $(GROUP_PPR) 6755 $(HOMEDIR)/bin $(BIN_PROGS_SUID)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755  $(HOMEDIR)/bin $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755  $(HOMEDIR)/lib $(LIB_PROGS) 
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 644  $(HOMEDIR)/fixup $(FIXUP_SCRIPTS)

include .depend

# Explicit rule because of System V release 4.0 make bugs.
ppr-xgrant$(DOTEXE): ppr-xgrant.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(CC) $(CFLAGS) -o ppr-xgrant$(DOTEXE) ppr-xgrant.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS)

# end of file

