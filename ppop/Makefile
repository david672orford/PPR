#
# mouse:~ppr/src/ppop/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 18 May 2006.
#

include ../Makefile.conf

#=== Inventory ===================================================================

BIN_PROGS=ppop$(DOTEXE)

#=== Build =======================================================================

all: $(BIN_PROGS) $(LIB_PROGS)

ppop$(DOTEXE): \
		ppop.o \
		ppop_cmds_listq.o \
		ppop_cmds_other.o \
		ppop_modify.o \
		dispatch_table.o \
		../libppr.a \
		../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)
	$(CHMOD) 4755 $@

# Extract an XML description of the commands from comments in the
# C source code.
dispatch_table.xml: ppop.c ppop_cmds_listq.c ppop_cmds_other.c ppop_modify.c
	( echo "<?xml version=\"1.0\"?>"; \
		 echo "<dispatch>"; \
		 sed -n -e '/^<command.*>/,/^<\/command>/ p' -e '/^<helptopic.*>/,/^<\/helptopic>/ p' *.c; \
		 echo "</dispatch>" \
	) >dispatch_table.xml

# Convert an XML description of subcommands into C source code
# for a table which describes them.
dispatch_table.h: dispatch_table.xml ../libppr/dispatch_table.xsl
	xsltproc ../libppr/dispatch_table.xsl dispatch_table.xml >dispatch_table.c

#=== Install =====================================================================

install: $(BIN_PROGS) $(LIB_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6711 $(BINDIR) $(BIN_PROGS)

#=== Housekeeping ================================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.o $(BIN_PROGS) $(LIB_PROGS)

# end of file

