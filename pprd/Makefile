#
# mouse:~ppr/src/pprd/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 25 April 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=pprd$(DOTEXE) pprd-question

#=== Build ==================================================================

all: $(PROGS)

pprd$(DOTEXE): \
		pprd.o pprd_log.o pprd_queue.o \
		pprd_respond.o pprd_alert.o pprd_ppop.o \
		pprd_remind.o pprd_destid.o \
		pprd_mainsup.o pprd_load.o \
		pprd_statedirs.o pprd_state.o \
		pprd_pprdrv.o pprd_printer.o \
		pprd_media.o \
		pprd_listener.o \
		pprd_question.o pprd_ipp.o \
		../libppr.a ../libgu.a 
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS) $(ZLIBLIBS) $(SOCKLIBS)

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
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(BINDIR) pprd$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR) pprd-question

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) *.o $(BACKUPS) $(PROGS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file
