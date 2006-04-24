#
# mouse:~ppr/src/ipp/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 24 April 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BIN_PROGS=ppr-xml-ipp-client$(DOTEXE)

CGI_PROGS=ipp$(DOTEXE)

PROGS=$(BIN_PROGS) $(CGI_PROGS)

#=== Build ==================================================================

all: $(PROGS)

ipp$(DOTEXE): ipp.o ipp_print.o ipp_destinations.o ipp_jobs.o ipp_cups_admin.o ipp_run.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(ZLIBLIBS)

ppr-xml-ipp-client$(DOTEXE): ppr-xml-ipp-client.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) `xml2-config --libs` -l cups -o $@ $^

ppr-xml-ipp-client.o: ppr-xml-ipp-client.c
	$(CC) $(CFLAGS) `xml2-config --cflags` -c ppr-xml-ipp-client.c

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(CGI_BIN) $(CGI_PROGS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.o $(BACKUPS) $(PROGS)

# end of file

