#
# mouse:~ppr/src/makeprogs/Makefile
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 7 March 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

TARGETS=ppr-config$(DOTEXE) paths.mk paths.sh paths.ph PPR.pm paths.tcl \
	squeeze$(DOTEXE)

#=== Build ==================================================================

all: $(TARGETS) version.mk

version.mk: ppr-config$(DOTEXE)
	./ppr-config --mk | grep SHORT_VERSION >version.mk

paths.mk: ppr-config$(DOTEXE)
	./ppr-config --mk >paths.mk

paths.sh: ppr-config$(DOTEXE)
	./ppr-config --sh >paths.sh

paths.ph: ppr-config$(DOTEXE)
	./ppr-config --ph >paths.ph

PPR.pm: ppr-config$(DOTEXE)
	./ppr-config --pm >PPR.pm

paths.tcl: ppr-config$(DOTEXE)
	./ppr-config --tcl >paths.tcl

#=== Install ================================================================

install: all
	$(INSTALLDATA) $(HOMEDIR)/lib paths.ph PPR.pm
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin ppr-config$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(TARGETS)
	$(RMF) installed_files_list
	# We need an empty file so make will run to build it.
	touch paths.mk

# end of file
