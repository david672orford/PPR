#
# mouse:~ppr/src/dotmatrix/Makefile
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 30 June 2000.
#

include ../makeprogs/global.mk

TARGETS=filter_dotmatrix$(DOTEXE) \
	TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0 \
	TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0 \
	TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0 \
	TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0

FILTER_OBJS=main.$(OBJ) linebuf.$(OBJ) postscript.$(OBJ) \
	tabbing.$(OBJ) graphics.$(OBJ) \
	pass1.$(OBJ) inbuf.$(OBJ) \
	escape_pass1.$(OBJ) escape_pass2.$(OBJ) \
	prop_tables.$(OBJ)

all: $(TARGETS)

install: $(TARGETS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/filters filter_dotmatrix$(DOTEXE)
	$(INSTALLCP) --squeeze TrinColl-PPR-Dotmatrix-1-2 $(SHAREDIR)/cache/procset/TrinColl-PPR-Dotmatrix-1-2
	$(INSTALLCP) --squeeze TrinColl-PPR-Dotmatrix-G1-1-0 $(SHAREDIR)/cache/procset/TrinColl-PPR-Dotmatrix-G1-1-0
	$(INSTALLCP) --squeeze TrinColl-PPR-Dotmatrix-G2-1-0 $(SHAREDIR)/cache/procset/TrinColl-PPR-Dotmatrix-G2-1-0
	$(INSTALLCP) --squeeze TrinColl-PPR-Dotmatrix-Colour-1-0 $(SHAREDIR)/cache/procset/TrinColl-PPR-Dotmatrix-Colour-1-0
	$(INSTALLCP) TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0 $(SHAREDIR)/cache/procset/TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0
	$(INSTALLCP) TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0 $(SHAREDIR)/cache/procset/TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0
	$(INSTALLCP) TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0 $(SHAREDIR)/cache/procset/TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0
	$(INSTALLCP) TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0 $(SHAREDIR)/cache/procset/TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0

include .depend

filter_dotmatrix$(DOTEXE): $(FILTER_OBJS) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(CC) $(CFLAGS) -o filter_dotmatrix$(DOTEXE) $(FILTER_OBJS) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)

# The file escape.c is compiled twice.  The general rules
# work for one, but for this one we need to specify -DPASS1.
escape_pass1.$(OBJ): escape.c filter_dotmatrix.h
	$(CC) $(CFLAGS) -DPASS1 -o escape_pass1.$(OBJ) -c escape.c

escape_pass2.$(OBJ): escape.c filter_dotmatrix.h
	$(CC) $(CFLAGS) -o escape_pass2.$(OBJ) -c escape.c

# We must rename this object file so it won't be
# destroyed when we build prop_genmetrics.
prop_tables.$(OBJ): prop.c filter_dotmatrix.h
	$(CC) $(CFLAGS) -c prop.c
	$(MV) prop.$(OBJ) prop_tables.$(OBJ)

# Compile the metric module again as a standalone program.
prop_genmetrics$(DOTEXE): prop.c filter_dotmatrix.h ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(CC) $(CFLAGS) -DGENMETRICS -o prop_genmetrics$(DOTEXE) prop.c ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)

TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0: prop_genmetrics$(DOTEXE)
	./prop_genmetrics ../vendors/font_ibm/IBMCourier.afm Courier 0 >TrinColl-PPR-Dotmatrix-MetricsEpson1-1-0

TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0: prop_genmetrics$(DOTEXE)
	./prop_genmetrics ../vendors/font_ibm/IBMCourier-Bold.afm Courier-Bold 0 >TrinColl-PPR-Dotmatrix-MetricsEpson2-1-0

TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0: prop_genmetrics$(DOTEXE)
	./prop_genmetrics ../vendors/font_ibm/IBMCourier-Italic.afm Courier-Oblique 1 >TrinColl-PPR-Dotmatrix-MetricsEpson3-1-0

TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0: prop_genmetrics$(DOTEXE)
	./prop_genmetrics ../vendors/font_ibm/IBMCourier-BoldItalic.afm Courier-BoldOblique 1 >TrinColl-PPR-Dotmatrix-MetricsEpson4-1-0

clean:
	$(RMF) $(BACKUPS) *.$(OBJ)
	$(RMF) $(TARGETS) prop_genmetrics$(DOTEXE)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file
