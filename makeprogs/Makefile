#
# mouse:~ppr/src/makeprogs/Makefile
# Copyright 1995--2001, Trinity College Computing Center.
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
# Last modified 6 December 2001.
#

include ../makeprogs/global.mk

#=== Inventory ==============================================================

TARGETS=gen_paths$(DOTEXE) paths.ph paths.mk paths.sh PPR.pm squeeze$(DOTEXE)

#=== Build ==================================================================

all: $(TARGETS)

paths.ph: gen_paths$(DOTEXE)
	./gen_paths --ph >paths.ph

PPR.pm: gen_paths$(DOTEXE)
	./gen_paths --pm >PPR.pm

paths.mk: gen_paths$(DOTEXE)
	./gen_paths --mk >paths.mk

paths.sh: gen_paths$(DOTEXE)
	./gen_paths --sh >paths.sh

configure: configure.in
	autoheader
	autoconf configure.in >configure
	chmod 755 configure

#=== Install ================================================================

install: all
	$(RMF) installed_files_list
	./make_install_dirs.sh
	$(INSTALLDATA) $(HOMEDIR)/lib paths.ph PPR.pm

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(TARGETS)
	$(RMF) installed_files_list
	touch paths.mk

clean-configure:
	$(RMF) configure config.log config.cache config.status config.h

# end of file

