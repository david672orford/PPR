#
# mouse:~ppr/src/uprint/Makefile
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 18 February 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=uprint-lpq$(DOTEXE) uprint-lprm$(DOTEXE) uprint-cancel$(DOTEXE) uprint-lpstat$(DOTEXE)
SETUID_PROGS=uprint-lpr$(DOTEXE) uprint-lp$(DOTEXE) 

PRIVATE_LIBS=../libuprint.$(LIBEXT) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)

PUBLIC_LIBS=$(SOCKLIBS) $(INTLLIBS)

LINK_LIBS=$(PRIVATE_LIBS) $(PUBLIC_LIBS)

#=== Build ==================================================================

all: $(PROGS) $(SETUID_PROGS)

uprint-lp$(DOTEXE): uprint-lp.$(OBJ) $(PRIVATE_LIBS)
	$(LD) $(LDFLAGS) -o $@ uprint-lp.$(OBJ) $(LINK_LIBS)

uprint-lpr$(DOTEXE): uprint-lpr.$(OBJ) $(PRIVATE_LIBS)
	$(LD) $(LDFLAGS) -o $@ uprint-lpr.$(OBJ) $(LINK_LIBS)

uprint-lpq$(DOTEXE): uprint-lpq.$(OBJ) $(PRIVATE_LIBS)
	$(LD) $(LDFLAGS) -o $@ uprint-lpq.$(OBJ) $(LINK_LIBS)

uprint-lprm$(DOTEXE): uprint-lprm.$(OBJ) $(PRIVATE_LIBS)
	$(LD) $(LDFLAGS) -o $@ uprint-lprm.$(OBJ) $(LINK_LIBS)

uprint-cancel$(DOTEXE): uprint-cancel.$(OBJ) $(PRIVATE_LIBS)
	$(LD) $(LDFLAGS) -o $@ uprint-cancel.$(OBJ) $(LINK_LIBS)

uprint-lpstat$(DOTEXE): uprint-lpstat.$(OBJ) $(PRIVATE_LIBS)
	$(LD) $(LDFLAGS) -o $@ uprint-lpstat.$(OBJ) $(LINK_LIBS)

#=== Install ================================================================

install: $(PROGS) $(SETUID_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 4755 $(HOMEDIR)/bin $(SETUID_PROGS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS) $(SETUID_PROGS)

# end of file
