#
# mouse:~ppr/src/papd/Makefile
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

BIN_PROGS=papd$(DOTEXE)

#=== Build ==================================================================

# If we aren't building with AppleTalk support, then ALL_ATALK will be 
# undefined and this will do nothing.  If we are building with AppleTalk
# support then it will be set to "allatalk".
all: $(ALL_ATALK)

# List if things to build if we have AppleTalk.
all-atalk: $(BIN_PROGS)

# This is the server.
papd$(DOTEXE): \
		papd.o \
		papd_$(ATALKTYPE).o \
		papd_conf.o \
		papd_query.o \
		papd_printjob.o \
		papd_login_aufs.o \
		papd_login_rbi.o \
		../libppr.a \
		../libgu.a \
		../libpprdb.a
	$(LD) $(LDFLAGS) -o $@ $^ $(ATALKLIBS) $(INTLLIBS) $(ZLIBLIBS)
	$(CHMOD) 6755 $@

# This one module might need extra flags to find the AppleTalk library 
# include files.
papd_$(ATALKTYPE).o: papd_$(ATALKTYPE).c
	$(CC) $(ATALKFLAGS) $(CFLAGS) -c $*.c

#=== Install ================================================================

# The install target.  This also does nothing if we
# don't have AppleTalk.
install: $(INSTALL_ATALK)

# List if things to install if we have AppleTalk.
install-atalk: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 4755 $(BINDIR) papd

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.o $(BACKUPS) $(BIN_PROGS)

# end of file
