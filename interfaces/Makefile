#
# mouse:~ppr/src/interfaces/Makefile
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 10 December 2004.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=dummy simple parallel$(DOTEXE) serial$(DOTEXE) \
	tcpip$(DOTEXE) lpr$(DOTEXE) smb usb$(DOTEXE) pros$(DOTEXE)

ATALK_PROGS=atalk$(DOTEXE)

NETBIOS_PROGS=clispool$(DOTEXE)

#=== Build ==================================================================

all: $(ALL_NETBIOS) $(ALL_ATALK) $(PROGS)

# Extra stuff to build if we have NetBIOS.
all-netbios: $(NETBIOS_PROGS)

# Extra stuff to build if we have AppleTalk.
all-atalk: $(ATALK_PROGS)

# Build the AppleTalk interface by linking the atalk.o created by the rule
# below with the AppleTalk libraries for this system as defined in
# Makefile.conf as ATALKLIBS.
atalk$(DOTEXE): atalk_$(ATALKTYPE).o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(ATALKLIBS)

atalk_$(ATALKTYPE).o: atalk_$(ATALKTYPE).c
	$(CC) $(ATALKFLAGS) $(CFLAGS) -c $*.c

# This target will only be compiled on AT&T systems with STREAMS NetBIOS.
clispool$(DOTEXE): clispool.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ -lnsl $(INTLLIBS)

# Berkely LPR/LPD protocol client:
lpr$(DOTEXE): lpr.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(SOCKLIBS) $(INTLLIBS)

# Parallel port interface:
parallel$(DOTEXE): parallel.o parallel_$(PARALLEL).o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

# Serial port interface
serial$(DOTEXE): serial.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

# HP JetDirect style TCP/IP interface
tcpip$(DOTEXE): tcpip.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(SOCKLIBS) $(INTLLIBS)

# USB printer
usb$(DOTEXE): usb.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

# Axis PROS protocol
pros$(DOTEXE): pros.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(SOCKLIBS) $(INTLLIBS)

#=== Install ================================================================

install: $(INSTALL_NETBIOS) $(INSTALL_ATALK) $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(INTDIR) $(PROGS)
	$(INSTALLLN) tcpip$(DOTEXE) $(INTDIR)/socketapi$(DOTEXE)
	$(INSTALLLN) tcpip$(DOTEXE) $(INTDIR)/appsocket$(DOTEXE)
	$(INSTALLLN) tcpip$(DOTEXE) $(INTDIR)/jetdirect$(DOTEXE)
	-chown root $(INTDIR)/lpr$(DOTEXE) && chmod 4711 $(INTDIR)/lpr$(DOTEXE)

# Extra stuff to install if we have AppleTalk.
install-atalk: $(ATALK_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(INTDIR) $(ATALK_PROGS)

# Extra stuff to install if we have NetBIOS.
install-netbios: $(NETBIOS_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(INTDIR) $(NETBIOS_PROGS)

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) *.o $(BACKUPS) $(PROGS) atalk$(DOTEXE) clispool$(DOTEXE)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file

