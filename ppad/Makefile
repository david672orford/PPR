#
# mouse:~ppr/src/ppad/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
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
# Last modified 8 February 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BIN_PROGS=ppad$(DOTEXE)
CONF_DATA=media.sample

#=== Build ==================================================================

all: $(BIN_PROGS)

ppad$(DOTEXE): \
		ppad.o \
		ppad_conf.o \
		ppad_util.o \
		ppad_printer.o \
		ppad_media.o \
		ppad_group.o \
		ppad_alias.o \
		ppad_ppd.o \
		dispatch_table.o \
		../libppr.a \
		../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS) $(ZLIBLIBS)
	chmod 4755 $@

# Extract an XML description of the commands from comments in the
# C source code.
dispatch_table.xml: ppad.c ppad_media.c ppad_ppd.c ppad_printer.c ppad_group.c ppad_alias.c
	( echo "<?xml version=\"1.0\"?>"; echo "<dispatch>"; sed -n -e '/^<command>/,/^<\/command>/ p' *.c; echo "</dispatch>" ) >dispatch_table.xml

# Convert an XML description of subcommands into C source code
# for a table which describes them.
dispatch_table.h: dispatch_table.xml ../libppr/dispatch_table.xsl
	xsltproc ../libppr/dispatch_table.xsl dispatch_table.xml >dispatch_table.c

#=== Install ================================================================

install: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6755 $(BINDIR) $(BIN_PROGS)
	$(INSTALLDATA) $(CONFDIR) $(CONF_DATA)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.o $(BACKUPS) $(BIN_PROGS)

# end of file
