#
# mouse:~ppr/src/pprd/Makefile
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 5 December 2001.
#

include ../makeprogs/global.mk

#=== Inventory ==============================================================

PROGS=pprd$(DOTEXE) pprd-kill

OBJS=pprd.$(OBJ) pprd_log.$(OBJ) pprd_queue.$(OBJ) \
	pprd_respond.$(OBJ) pprd_alert.$(OBJ) pprd_ppop.$(OBJ) \
	pprd_remind.$(OBJ) pprd_nodeid.$(OBJ) pprd_destid.$(OBJ) \
	pprd_mainsup.$(OBJ) pprd_load.$(OBJ) pprd_pprdrv.$(OBJ) \
	pprd_media.$(OBJ) pprd_printer.$(OBJ) pprd_state.$(OBJ) \
	pprd_remote.$(OBJ)

#=== Build ==================================================================

all: $(PROGS)

pprd$(DOTEXE): $(OBJS) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o pprd$(DOTEXE) $(OBJS) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)
	$(CHMOD) 6755 pprd$(DOTEXE)

# This automatically makes a header file which has prototypes for
# all functions and has extern definitions of all global variables.
# This file is not updated automatically, you must remove pprd.auto_h
# if a function prototype changes.
pprd.auto_h:
	# functions: eliminate static, ???, ???
	cat *.c | egrep -v '^static ' \
		| egrep '^[^ 	]+ [^ ]* *[^ ]+\(.*\)$$' \
		| sed -e 's/^\(.*\)$$/\1;/' >pprd.auto_h
	# global variables: eliminate static, add "extern", remove size
	cat *.c | egrep -v '^static ' \
		| sed -n -e 's/^\([^ 	\*][^;(]*;\).*$$/extern \1/p' \
		| sed -e 's/\[[^[]*\]/[]/' -e 's/=[^;]*;/;/' \
			 >>pprd.auto_h

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6755 $(HOMEDIR)/bin pprd
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR)  755 $(HOMEDIR)/bin pprd-kill

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) pprd$(DOTEXE)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file
