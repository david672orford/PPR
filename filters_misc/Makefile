#
# mouse:~ppr/src/filters_misc/Makefile
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
# Last modified 18 April 2005.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BUILT=filter_hexdump$(DOTEXE) make_dvips_conf$(DOTEXE) indexfilters

SCRIPTS_SH=troff_real.sh ditroff_real.sh troff_groff.sh ditroff_groff.sh \
	dvi.sh texinfo.sh \
	jpeg.sh gif.sh bmp.sh pnm.sh xbm.sh xpm.sh xwd.sh tiff.sh png.sh \
	plot_postplot.sh plot_plot2ps.sh fig.sh \
	pdf_acroread.sh pdf_xpdf.sh \
	html_htmldoc.sh

SCRIPTS_PERL=tex.perl

SCRIPTS_TCL=pr.tcl

#=== Build ==================================================================

all: $(BUILT)

filter_hexdump$(DOTEXE): filter_hexdump.o ../libgu.a ../libppr.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

make_dvips_conf$(DOTEXE): make_dvips_conf.o ../libgu.a ../libppr.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

#=== Install ================================================================

install: $(BUILT) $(SCRIPTS_SH) $(SCRIPTS_PERL) $(SCRIPTS_TCL)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR)  755 $(FILTDIR) filter_hexdump$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 2755 $(LIBDIR) make_dvips_conf$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR)  755 $(LIBDIR) indexfilters
	$(INSTALLDATA) $(MISCDIR) mfmodes.conf
	for f in $(SCRIPTS_SH);\
		do\
		$(INSTALLCP) $$f $(LIBDIR)/fixup/filter_`basename $$f .sh`;\
		done
	for f in $(SCRIPTS_PERL);\
		do\
		$(INSTALLCP) $$f $(LIBDIR)/fixup/filter_`basename $$f .perl`;\
		done
	for f in $(SCRIPTS_TCL);\
		do\
		$(INSTALLCP) $$f $(LIBDIR)/fixup/filter_`basename $$f .tcl`;\
		done
	#./ppr-index filters >/dev/null

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.o $(BACKUPS) $(BUILT)

# end of file
