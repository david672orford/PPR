#
# mouse:~ppr/src/pprpopup/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 27 August 2002.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BUILT_PROGS=pprpopup.tcl
UNBUILD_PROGS=ppr-popup
PROGS=$(BUILT_PROGS) $(UNBUILT_PROGS)
IMAGES=splashscreen.tid icon_frown.tid icon_stopsign.tid

#=== Build ==================================================================

all: $(PROGS)

pprpopup.tcl: pprpopup_main.tcl browser.itk urlfetch.itk
	echo "#! /usr/bin/wish" >pprpopup.tcl
	cat urlfetch.itk browser.itk md5pure.tcl pprpopup_main.tcl \
		| sed -e '/^[ 	]*#/d' -e 's/^[ 	][ 	]*//' -e '/^$$/d' \
		>>pprpopup.tcl

.SUFFIXES: .fig .tid

images: $(IMAGES)

.fig.tid:
	./fig2tid $*

#=== Install ================================================================

install: $(PROGS)
	$(MAKE_NEW_DIR) $(SHAREDIR)/www/clientsoft
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(SHAREDIR)/www/clientsoft pprpopup.tcl
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin ppr-popup

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(BUILT_PROGS)

# end of file
