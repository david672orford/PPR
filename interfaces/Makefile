#
# mouse:~ppr/src/interfaces/Makefile
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
# Last modified 23 October 2002.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=dummy simple parallel$(DOTEXE) serial$(DOTEXE) \
	tcpip$(DOTEXE) lpr$(DOTEXE) smb

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

