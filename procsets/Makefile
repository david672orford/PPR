#
# mouse:~ppr/src/procsets/Makefile
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
# Last modified 28 February 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=

SQUEEZEME=\
	TrinColl-PPR-dmm-nup-3-6 \
	TrinColl-PPR-dmm-nup-3-7 \
	TrinColl-PPR-ReEncode-1.1-0 \
	TrinColl-PPR-NewMetrics-1-0 \
	TrinColl-PPR-UNICODE-1-0

#=== Build ==================================================================

all: $(PROGS)

#=== Install ================================================================

install: $(PROGS) $(SQUEEZEME)
	for i in $(SQUEEZEME);\
	    do\
	    $(INSTALLCP) --squeeze $$i $(RESOURCEDIR)/procset/$$i;\
	    done

#=== Housekeeping ===========================================================

depend:

clean:
	$(RMF) $(BACKUPS) $(PROGS)

# end of file
