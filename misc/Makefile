#
# mouse:~ppr/src/misc/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 17 August 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=ppr-sync \
	ppd2macosdrv \
	custom_hook_docutech \
	xmessage \
	ppr-testpage

#=== Build ==================================================================

all: $(PROGS)

xmessage: xmessage.tcl
	cp xmessage.tcl xmessage
	$(CHMOD) 755 xmessage

ppr-testpage$(DOTEXE): ppr-testpage.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(BINDIR) ppr-sync ppd2macosdrv ppr-testpage
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR) custom_hook_docutech xmessage

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.o $(BACKUPS) $(PROGS)

# end of file
