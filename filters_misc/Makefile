#
# mouse:~ppr/src/misc_filters/Makefile
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
# Last modified 1 June 2001.
#

include ../makeprogs/global.mk

PROGS=filter_hexdump$(DOTEXE) make_dvips_conf$(DOTEXE)

SCRIPTS_SH=pr.sh \
	troff_real.sh ditroff_real.sh troff_groff.sh ditroff_groff.sh \
	dvi.sh texinfo.sh \
	jpeg.sh gif.sh bmp.sh pnm.sh xbm.sh xpm.sh xwd.sh tiff.sh png.sh \
	plot_postplot.sh plot_plot2ps.sh fig.sh \
	pdf_acroread.sh pdf_xpdf.sh \
	html_htmldoc.sh

SCRIPTS_PERL=tex.perl

all: $(PROGS)

install: $(PROGS) $(SCRIPTS_SH) $(SCRIPTS_PERL)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/filters filter_hexdump$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 2755 $(HOMEDIR)/lib make_dvips_conf$(DOTEXE)
	$(INSTALLDATA) $(SHAREDIR)/lib mfmodes.conf
	for f in $(SCRIPTS_SH);\
		do\
		$(INSTALLCP) $$f $(HOMEDIR)/fixup/filter_`basename $$f .sh`;\
		done
	for f in $(SCRIPTS_PERL);\
		do\
		$(INSTALLCP) $$f $(HOMEDIR)/fixup/filter_`basename $$f .perl`;\
		done

include .depend

filter_hexdump$(DOTEXE): filter_hexdump.c ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(CC) $(CFLAGS) -o filter_hexdump$(DOTEXE) filter_hexdump.c ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)

make_dvips_conf$(DOTEXE): make_dvips_conf.c ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(CC) $(CFLAGS) -o make_dvips_conf$(DOTEXE) make_dvips_conf.c ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

# end of file

