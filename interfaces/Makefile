#
# mouse:~ppr/src/interfaces/Makefile
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
# Last modified 11 January 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=dummy simple parallel$(DOTEXE) serial$(DOTEXE) \
	tcpip$(DOTEXE) lpr$(DOTEXE) smb usblp$(DOTEXE)

ATALK_PROGS=atalk$(DOTEXE)

NETBIOS_PROGS=clispool$(DOTEXE)

#=== Build ==================================================================

all: $(ALL_NETBIOS) $(ALL_ATALK) $(PROGS)

# Extra stuff to build if we have NetBIOS.
all-netbios: $(NETBIOS_PROGS)

# Extra stuff to build if we have AppleTalk.
all-atalk: $(ATALK_PROGS)

# Build the AppleTalk interface by linking the atalk.o
# created by the rule below with the AppleTalk libraries
# for this system as defined in global.mk as ATALKLIBS.
atalk$(DOTEXE): atalk_$(ATALKTYPE).$(OBJ) throw.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)
	$(LD) $(LDFLAGS) -o atalk$(DOTEXE) atalk_$(ATALKTYPE).$(OBJ) \
		$(ATALKLIBS) \
		throw.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

atalk_$(ATALKTYPE).$(OBJ): atalk_$(ATALKTYPE).c
	$(CC) $(ATALKFLAGS) $(CFLAGS) -c atalk_$(ATALKTYPE).c

# This target will only be compiled on AT&T systems with STREAMS NetBIOS.
clispool$(DOTEXE): clispool.$(OBJ) throw.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o clispool$(DOTEXE) clispool.$(OBJ) \
		-lnsl \
		throw.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

# Berkely LPR/LPD protocol client:
lpr$(DOTEXE): lpr.$(OBJ) throw.$(OBJ) ../libuprint.$(LIBEXT) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o lpr$(DOTEXE) lpr.$(OBJ) \
		throw.$(OBJ) ../libuprint.$(LIBEXT) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(SOCKLIBS) $(INTLLIBS)

# Parallel port interface:
parallel$(DOTEXE): parallel.$(OBJ) parallel_$(PARALLEL).$(OBJ) throw.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o parallel$(DOTEXE) parallel.$(OBJ) parallel_$(PARALLEL).$(OBJ) \
		throw.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

# Serial port interface
serial$(DOTEXE): serial.$(OBJ) throw.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o serial$(DOTEXE) serial.$(OBJ) \
		throw.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

# HP JetDirect style TCP/IP interface
tcpip$(DOTEXE): tcpip.$(OBJ) throw.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o tcpip$(DOTEXE) tcpip.$(OBJ) \
		throw.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(SOCKLIBS) $(INTLLIBS)

# USB printer
usblp$(DOTEXE): usblp.$(OBJ) throw.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o usblp$(DOTEXE) usblp.$(OBJ) \
		throw.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

#=== Install ================================================================

install: $(INSTALL_NETBIOS) $(INSTALL_ATALK) $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/interfaces $(PROGS)
	-chown root $(HOMEDIR)/interfaces/lpr$(DOTEXE) && chmod 4711 $(HOMEDIR)/interfaces/lpr$(DOTEXE)

# Extra stuff to install if we have AppleTalk.
install-atalk: $(ATALK_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/interfaces $(ATALK_PROGS)

# Extra stuff to install if we have NetBIOS.
install-netbios: $(NETBIOS_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/interfaces $(NETBIOS_PROGS)

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS) atalk$(DOTEXE) clispool$(DOTEXE)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file

