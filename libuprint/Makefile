#
# mouse:~ppr/src/libuprint/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 17 May 2002.
#

include ../Makefile.conf
include ../makeprogs/paths.mk

#=== Inventory ==============================================================

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
USELIBS=../libuprint.$(LIBEXT) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)
DATA=uprint.conf.sample uprint-remote.conf.sample

#=== Build ==================================================================

all: $(TARGETS) $(DATA)

../libuprint.$(LIBEXT): $(LIBUPRINT_OBJS)
	$(RMF) ../libuprint.$(LIBEXT)
	-$(LIBCMD) ../libuprint.$(LIBEXT) >/dev/null 2>&1
	$(LIBCMD) ../libuprint.$(LIBEXT) $(LIBUPRINT_OBJS)

uprint-newconf$(DOTEXE): uprint-newconf.$(OBJ) $(USELIBS)
	$(CC) $(CFLAGS) -o uprint-newconf$(DOTEXE) uprint-newconf.$(OBJ) $(USELIBS) $(INTLLIBS)

#=== Install ================================================================

install: $(TARGETS) $(DATA)
	$(INSTALLDATA) $(CONFDIR) $(DATA)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(PROGS)

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(TARGETS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file

