#
# mouse:~ppr/src/lprsrv/Makefile
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
# Last modified 30 June 2000.
#

include ../makeprogs/global.mk

LIB_PROGS=olprsrv$(DOTEXE) lprsrv$(DOTEXE)
BIN_PROGS=lprsrv-test$(DOTEXE)
PROGS=$(BIN_PROGS) $(LIB_PROGS)

LPRSRV_OBJS=lprsrv.$(OBJ) lprsrv_client_info.$(OBJ) lprsrv_conf.$(OBJ) \
	lprsrv_print.$(OBJ) lprsrv_list.$(OBJ) lprsrv_cancel.$(OBJ) \
	lprsrv_standalone.$(OBJ)

all: $(PROGS)

install: $(PROGS) lprsrv.conf.sample
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib $(LIB_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(BIN_PROGS)
	$(INSTALLCP) lprsrv.conf.sample $(CONFDIR)/lprsrv.conf.sample

include .depend

olprsrv$(DOTEXE): olprsrv.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT) ../libuprint.$(LIBEXT)
	$(CC) $(CFLAGS) -o olprsrv$(DOTEXE) olprsrv.$(OBJ) \
		../libuprint.$(LIBEXT) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT) $(SOCKLIBS)

lprsrv$(DOTEXE): $(LPRSRV_OBJS) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT) ../libuprint.$(LIBEXT)
	$(CC) $(CFLAGS) -o lprsrv$(DOTEXE) $(LPRSRV_OBJS) \
		../libuprint.$(LIBEXT) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT) $(SOCKLIBS) $(INTLLIBS)

lprsrv-test$(DOTEXE): lprsrv-test.$(OBJ) lprsrv_conf.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(CC) $(CFLAGS) -o lprsrv-test$(DOTEXE) lprsrv-test.$(OBJ) lprsrv_conf.$(OBJ) \
		../libgu.$(LIBEXT) ../libppr.$(LIBEXT) $(SOCKLIBS) $(INTLLIBS)

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file

