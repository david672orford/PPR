#
# mouse:~ppr/src/ppad/Makefile
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
# Last modified 13 March 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BIN_PROGS=ppad$(DOTEXE)
CONF_DATA=media.sample

#=== Build ==================================================================

all: $(BIN_PROGS)

OBJS1=ppad.$(OBJ) \
	ppad_printer.$(OBJ) \
	ppad_media.$(OBJ) \
	ppad_group.$(OBJ) \
	ppad_conf.$(OBJ) \
	ppad_filt.$(OBJ) \
	ppad_util.$(OBJ) \
	ppad_alias.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
ppad$(DOTEXE): $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(INTLLIBS) $(ZLIBLIBS)
	chmod 4755 $@

#=== Install ================================================================

install: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6755 $(HOMEDIR)/bin $(BIN_PROGS)
	$(INSTALLDATA) $(CONFDIR) $(CONF_DATA)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(BIN_PROGS)

# end of file
