#
# mouse:~ppr/src/browsers/Makefile
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
# Last modified 1 September 2005.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BROWSERS=parallel ip_scan_snmp$(DOTEXE) usb$(DOTEXE)

#=== Build ==================================================================

all: $(BROWSERS) $(ALL_ATALK)

all-atalk: atalk nbp_lookup$(DOTEXE)

nbp_lookup$(DOTEXE): nbp_lookup_$(ATALKTYPE).o ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(ATALKLIBS)

ip_scan_snmp$(DOTEXE): ip_scan_snmp.o ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(SOCKLIBS)

usb$(DOTEXE): usb.o ../libgu.a ../libppr.a
	$(LD) $(LDFLAGS) -o $@ $^

#=== Install ================================================================

install: all $(INSTALL_ATALK)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR)/browsers $(BROWSERS)

install-atalk: $(ATALK_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR) nbp_lookup$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR)/browsers atalk$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.o $(BACKUPS) $(BROWSERS) atalk

# end of file

