#
# mouse:~ppr/src/pprd/Makefile
# Copyright 1995--2005, Trinity College Computing Center.
# Written by David Chappell.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
#
# Last modified 14 January 2005.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=pprd$(DOTEXE) pprd-question

#=== Build ==================================================================

all: $(PROGS)

pprd$(DOTEXE): \
		pprd.$(OBJ) pprd_log.$(OBJ) pprd_queue.$(OBJ) \
		pprd_respond.$(OBJ) pprd_alert.$(OBJ) pprd_ppop.$(OBJ) \
		pprd_remind.$(OBJ) pprd_destid.$(OBJ) \
		pprd_mainsup.$(OBJ) pprd_load.$(OBJ) pprd_pprdrv.$(OBJ) \
		pprd_media.$(OBJ) pprd_printer.$(OBJ) pprd_state.$(OBJ) \
		pprd_question.$(OBJ) pprd_ipp.$(OBJ) \
		../libppr.$(LIBEXT) ../libgu.$(LIBEXT) 
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)
	$(CHMOD) 6755 $@

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
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6755 $(BINDIR) pprd$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR)  755 $(LIBDIR) pprd-question

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file
