#
# mouse:~ppr/src/libuprint/Makefile
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
# Last modified 6 July 2000.
#

include ../makeprogs/global.mk
include ../makeprogs/paths.mk

LIBUPRINT_OBJS=lpr_connect.$(OBJ) lpr_failover.$(OBJ) lpr_sendfile.$(OBJ) \
	claim_ppr.$(OBJ) claim_lp.$(OBJ) claim_lpr.$(OBJ) \
	claim_remote.$(OBJ) uprint.$(OBJ) uprint_xlate.$(OBJ) \
	uprint_print.$(OBJ) \
	uprint_type_ppr.$(OBJ) uprint_argv_ppr.$(OBJ) \
	uprint_type_lpr.$(OBJ) uprint_argv_lpr.$(OBJ) \
	uprint_type_lp.$(OBJ) uprint_argv_lp.$(OBJ) \
	uprint_print_rfc1179.$(OBJ) \
	uprint_lpq.$(OBJ) uprint_lpq_rfc1179.$(OBJ) \
	uprint_lprm.$(OBJ) uprint_lprm_rfc1179.$(OBJ) \
	uprint_loop.$(OBJ) uprint_uid.$(OBJ) \
	uprint_run.$(OBJ) uprint_strerror.$(OBJ) \
	uprint_conf.$(OBJ) uprint_sysv.$(OBJ)

LIBS=../libuprint.$(LIBEXT)
PROGS=uprint-newconf$(DOTEXE)
TARGETS=$(LIBS) $(PROGS)

DATA=uprint.conf.sample uprint-remote.conf.sample

all: $(TARGETS) $(DATA)

install: $(TARGETS) $(DATA)
	$(INSTALLDATA) $(CONFDIR) $(DATA)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(PROGS)

include .depend

../libuprint.$(LIBEXT): $(LIBUPRINT_OBJS)
	$(RMF) ../libuprint.$(LIBEXT)
	$(LIBCMD) ../libuprint.$(LIBEXT) >/dev/null 2>&1
	$(LIBCMD) ../libuprint.$(LIBEXT) $(LIBUPRINT_OBJS)

uprint-newconf$(DOTEXE): uprint-newconf.$(OBJ) ../libuprint.$(LIBEXT) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)
	$(CC) $(CFLAGS) -o uprint-newconf$(DOTEXE) uprint-newconf.$(OBJ) ../libuprint.$(LIBEXT) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(TARGETS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file

