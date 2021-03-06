#
# mouse:~ppr/src/libscript/Makefile
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
# Last modified 19 October 2005.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BUILT_BINS=alert$(DOTEXE) ppr_conf_query$(DOTEXE) \
	rewind_stdin$(DOTEXE) tail_status$(DOTEXE) \
	file_outdated$(DOTEXE) mkstemp$(DOTEXE)

BUILT_LIBS=\
	respond.sh respond.ph \
	signal.sh \
	interface.sh interface.tcl \

UNBUILT_LIBS=\
	responder_argv.pl \
	speach.pl speach_play.pl \
	MD5pp.pm \
	readppd.pl \
	acl.pl

BUILT_OTHER=signal_sh$(DOTEXE)

PPR_LIBS=PPOP.pm

#=== Build ==================================================================

all: $(BUILT_BINS) $(BUILT_LIBS) $(UNBUILT_LIBS) $(PPR_LIBS)

# A program which calls alert().
alert$(DOTEXE): alert.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

# Program to get a value from ppr.conf.
ppr_conf_query$(DOTEXE): ppr_conf_query.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

# Program which prints PPR status message stream on stdout.
tail_status$(DOTEXE): tail_status.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS)

# Wrapper for getsockbyname().
getservbyname$(DOTEXE): getservbyname.o
	$(LD) $(LDFLAGS) -o $@ $^ $(SOCKLIBS)

# A wrapper for mkstemp().
mkstemp$(DOTEXE): mkstemp.o ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^

# Signal values in Borne shell script form.
signal.sh: signal_sh$(DOTEXE)
	./signal_sh >signal.sh

# Shell version of respond.h.
respond.sh: ../include/respond.h
	echo "# Bourne shell translation of respond.h" >respond.sh
	sed -n -e 's/^#define \([^ ]*\) \([0-9][0-9]*\)\([	]*\)\(.*\)$$/\1=\2\3#\4/p' \
		<../include/respond.h >>respond.sh

# Perl version of respond.h.
respond.ph: ../include/respond.h
	echo "# Perl translation of respond.h" >respond.ph
	sed -n -e 's/^#define \([^ ]*\) \([0-9][0-9]*\)\([ 	]*\)\(.*\)$$/$$\1=\2;\3#\4/p' \
		<../include/respond.h >>respond.ph

# Shell version of interface.h.
interface.sh: ../include/interface.h
	echo "# Bourne shell translation of interface.h" >interface.sh
	sed -n -e 's/^#define \([^ ]*\) \([0-9][0-9]*\)\([ 	]*\)\(.*\)$$/\1=\2\3#\4/p' \
		<../include/interface.h >>interface.sh

# Tcl version of interface.h.
interface.tcl: ../include/interface.h
	echo "# Tcl translation of interface.h" >interface.sh
	sed -n -e 's/^#define \([^ ]*\) \([0-9][0-9]*\)\([ 	]*\)\(.*\)$$/set \1 \2/p' \
		<../include/interface.h >>interface.tcl

#=== Install ================================================================

install: all
	$(INSTALLDATA) $(LIBDIR) $(BUILT_LIBS) $(UNBUILT_LIBS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR) $(BUILT_BINS)
	$(MAKE_NEW_DIR) $(LIBDIR)/PPR
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR)/PPR $(PPR_LIBS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.o $(BUILT_BINS) $(BUILT_LIBS) $(BUILT_OTHER)

# end of file

