#
# mouse:~ppr/src/libpprdb/gdbm/Makefile
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
# Last modified 17 May 2002.
#

include ../Makefile.conf

OBJS=$(USER_DBM)_dbauth.$(OBJ) $(USER_DBM)_dbtrans.$(OBJ) \
	$(USER_DBM)_dbmod.$(OBJ) dbstrlower.$(OBJ)

all: ../libpprdb.$(LIBEXT)

install: all

include .depend

../libpprdb.$(LIBEXT): $(OBJS)
	$(RMF) ../libpprdb.$(LIBEXT)
	-$(LIBCMD) ../libpprdb.$(LIBEXT) >/dev/null 2>&1
	$(LIBCMD) ../libpprdb.$(LIBEXT) $(OBJS)

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) ../libpprdb.$(LIBEXT)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file
