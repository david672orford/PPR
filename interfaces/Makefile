#
# mouse:~ppr/src/interfaces/Makefile
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
# Last modified 10 April 2001.
#

include ../makeprogs/global.mk

PROGS=dummy simple parallel$(DOTEXE) serial$(DOTEXE) \
	tcpip$(DOTEXE) lpr$(DOTEXE) smb \
	gssimple gsparallel gsserial gsatalk gstcpip \
	gslpr gssmb

ATALK_PROGS=atalk$(DOTEXE)

NETBIOS_PROGS=clispool$(DOTEXE)

# This is the Sed command to process gs_master.sh after it
# has passed thru cpp.  It is quoted a bit to get thru
# make and sh.
SEDCMD=./sedcmd.sh

all: $(ALLNETBIOS) $(ALLATALK) $(PROGS)

# Extra stuff to build if we have NetBIOS.
allnetbios: $(NETBIOS_PROGS)

# Extra stuff to build if we have AppleTalk.
allatalk: $(ATALK_PROGS)

install: $(INSTALLNETBIOS) $(INSTALLATALK) $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/interfaces $(PROGS)
	-chown root $(HOMEDIR)/interfaces/lpr$(DOTEXE) && chmod 4711 $(HOMEDIR)/interfaces/lpr$(DOTEXE)

# Extra stuff to install if we have AppleTalk.
installatalk: $(ATALK_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/interfaces $(ATALK_PROGS)

# Extra stuff to install if we have NetBIOS.
installnetbios: $(NETBIOS_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/interfaces $(NETBIOS_PROGS)

include .depend

# Build the AppleTalk interface by linking the atalk.o
# created by the rule below with the AppleTalk libraries
# for this system as defined in global.mk as ATALKLIBS.
atalk$(DOTEXE): atalk_$(ATALKTYPE).$(OBJ) throw.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)
	$(LD) $(LDFLAGS) -o atalk$(DOTEXE) atalk_$(ATALKTYPE).$(OBJ) \
		$(ATALKLIBS) \
		throw.$(OBJ) \
		../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

atalk_$(ATALKTYPE).$(OBJ): atalk_$(ATALKTYPE).c
	$(CC) $(ATALKFLAGS) $(CFLAGS) -c atalk_$(ATALKTYPE).c

# This target will only be compiled on AT&T systems with STREAMS NetBIOS.
clispool$(DOTEXE): clispool.$(OBJ) throw.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o clispool$(DOTEXE) clispool.$(OBJ) \
		-lnsl \
		throw.$(OBJ) \
		../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

# Berkely LPR/LPD protocol client:
lpr$(DOTEXE): lpr.$(OBJ) throw.$(OBJ) ../libuprint.$(LIBEXT) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o lpr$(DOTEXE) lpr.$(OBJ) \
		throw.$(OBJ) \
		../libuprint.$(LIBEXT) \
		../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(SOCKLIBS) $(INTLLIBS)

# Parallel port interface:
parallel$(DOTEXE): parallel.$(OBJ) parallel_$(PARALLEL).$(OBJ) throw.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o parallel$(DOTEXE) parallel.$(OBJ) parallel_$(PARALLEL).$(OBJ) \
		throw.$(OBJ) \
		../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

# Serial port interface
serial$(DOTEXE): serial.$(OBJ) throw.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o serial$(DOTEXE) serial.$(OBJ) \
		throw.$(OBJ) \
		../libppr.$(LIBEXT) ../libgu.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

# HP JetDirect style TCP/IP interface
tcpip$(DOTEXE): tcpip.$(OBJ) throw.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o tcpip$(DOTEXE) tcpip.$(OBJ) \
		throw.$(OBJ) \
		../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(SOCKLIBS) $(INTLLIBS)

gssimple.sh: gs_master.sh
	$(CPP) -DBACKEND_SIMPLE gs_master.sh | $(SEDCMD) >gssimple.sh

gsparallel.sh: gs_master.sh
	$(CPP) -DBACKEND_PARALLEL gs_master.sh | $(SEDCMD) >gsparallel.sh

gsserial.sh: gs_master.sh
	$(CPP) -DBACKEND_SERIAL gs_master.sh | $(SEDCMD) >gsserial.sh

gsatalk.sh: gs_master.sh
	$(CPP) -DBACKEND_ATALK gs_master.sh | $(SEDCMD) >gsatalk.sh

gstcpip.sh: gs_master.sh
	$(CPP) -DBACKEND_TCPIP gs_master.sh | $(SEDCMD) >gstcpip.sh

gslpr.sh: gs_master.sh
	$(CPP) -DBACKEND_LPR gs_master.sh | $(SEDCMD) >gslpr.sh

gssmb.sh: gs_master.sh
	$(CPP) -DBACKEND_SMB gs_master.sh | $(SEDCMD) >gssmb.sh

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS) atalk$(DOTEXE) clispool$(DOTEXE)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file

