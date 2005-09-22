#
# mouse:~ppr/src/filter_dotmatrix/Makefile
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

TARGETS=filter_dotmatrix$(DOTEXE) \
	TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0 \
	TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0 \
	TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0 \
	TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0


#=== Build ==================================================================

all: $(TARGETS)

filter_dotmatrix$(DOTEXE): \
		main.o linebuf.o postscript.o \
		tabbing.o graphics.o \
		pass1.o inbuf.o \
		escape_pass1.o escape_pass2.o \
		prop_tables.o \
		../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

# The file escape.c is compiled twice.  The general rules
# work for one, but for this one we need to specify -DCOMPILING_PASS1.
escape_pass1.o: escape.c filter_dotmatrix.h
	$(CC) $(CFLAGS) -DCOMPILING_PASS1 -o $@ -c escape.c

escape_pass2.o: escape.c filter_dotmatrix.h
	$(CC) $(CFLAGS)                   -o $@ -c escape.c

# Compile the metric module a first time to generate tables which we can
# compile into the program.  We must list filter_dotmatrix.h separately
# because prop_tables.o isn't listed in .depend.
prop_tables.o: prop.c filter_dotmatrix.h
	$(CC) $(CFLAGS) -o $@ -c prop.c

# Compile the metric module again as a standalone program.
prop_genmetrics.o: prop.c filter_dotmatrix.h
	$(CC) $(CFLAGS) -DGENMETRICS -o $@ -c prop.c

prop_genmetrics$(DOTEXE): prop_genmetrics.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^

# Use the prop_genmetrics program to generate metrics tables from the
# fonts' AFM files.
TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0: prop_genmetrics$(DOTEXE)
	./prop_genmetrics ../nonppr_vendors/font_ibm/IBMCourier.afm Courier 0 >TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0

TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0: prop_genmetrics$(DOTEXE)
	./prop_genmetrics ../nonppr_vendors/font_ibm/IBMCourier-Bold.afm Courier-Bold 0 >TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0

TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0: prop_genmetrics$(DOTEXE)
	./prop_genmetrics ../nonppr_vendors/font_ibm/IBMCourier-Italic.afm Courier-Oblique 1 >TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0

TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0: prop_genmetrics$(DOTEXE)
	./prop_genmetrics ../nonppr_vendors/font_ibm/IBMCourier-BoldItalic.afm Courier-BoldOblique 1 >TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0

#=== Install ================================================================

install: $(TARGETS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(FILTDIR) filter_dotmatrix$(DOTEXE)
	$(INSTALLCP) --squeeze TrinColl-PPR-Dotmatrix-1-2 $(RESOURCEDIR)/procset/TrinColl-PPR-Dotmatrix-1-2
	$(INSTALLCP) --squeeze TrinColl-PPR-Dotmatrix-G1-1-0 $(RESOURCEDIR)/procset/TrinColl-PPR-Dotmatrix-G1-1-0
	$(INSTALLCP) --squeeze TrinColl-PPR-Dotmatrix-G2-1-0 $(RESOURCEDIR)/procset/TrinColl-PPR-Dotmatrix-G2-1-0
	$(INSTALLCP) --squeeze TrinColl-PPR-Dotmatrix-Colour-1-0 $(RESOURCEDIR)/procset/TrinColl-PPR-Dotmatrix-Colour-1-0
	$(INSTALLCP) TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0 $(RESOURCEDIR)/procset/TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0
	$(INSTALLCP) TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0 $(RESOURCEDIR)/procset/TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0
	$(INSTALLCP) TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0 $(RESOURCEDIR)/procset/TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0
	$(INSTALLCP) TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0 $(RESOURCEDIR)/procset/TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) $(BACKUPS) *.o
	$(RMF) $(TARGETS) prop_genmetrics$(DOTEXE)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file
