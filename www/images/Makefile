#
# mouse:~ppr/src/www/images/Makefile
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
# Last modified 23 January 2004.
#

include ../../Makefile.conf

#=== Inventory ==============================================================

GENERATED_IMAGES=pprlogo2.eps \
	pprlogo2-large.png pprlogo2-medium.png pprlogo2-small.png \
	prn_testpage1.png prn_testpage2.png \
	cliconf1.png \
	exclaim.png

#=== Build ==================================================================

all: $(GENERATED_IMAGES)

.SUFFIXES: .fig .png

.fig.png:
	./fig2png --wiz $*.fig $*.png

pprlogo2.eps: pprlogo2.fig
	fig2dev -L eps pprlogo2.fig >pprlogo2.eps

pprlogo2-large.png: pprlogo2.fig
	./fig2png pprlogo2.fig $*.png

pprlogo2-medium.png: pprlogo2.fig
	./fig2png --half pprlogo2.fig $*.png

pprlogo2-small.png: pprlogo2.fig
	./fig2png --quarter pprlogo2.fig $*.png

#=== Install ================================================================

install:

#=== Housekeeping ===========================================================

depend:

clean:
	$(RMF) *.o $(BACKUPS) $(BIN_PROGS)

veryclean: clean
	$(RMF) $(GENERATED_IMAGES)

# end of file

