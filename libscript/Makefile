#
# mouse:~ppr/src/libscript/Makefile
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
# Last modified 4 May 2001.
#

include ../makeprogs/global.mk

#=== Inventory ==============================================================

BUILT_BINS=alert$(DOTEXE) ppr_conf_query$(DOTEXE) time_elapsed$(DOTEXE) \
	rewind_stdin$(DOTEXE) tail_status$(DOTEXE) \
	getpwnam$(DOTEXE) getgrnam$(DOTEXE) getservbyname$(DOTEXE) \
	file_outdated$(DOTEXE) mkstemp$(DOTEXE)

BUILT_LIBS=respond.sh respond.ph signal.sh commentary.sh commentary.ph

UNBUILT_LIBS=interface.sh libppr_int.sh \
	responder_argv.pl pprpopup.pl \
	speach.pl speach_play.pl speach_commentary.pl speach_response.pl \
	MD5pp.pm readppd.pl acl.pl

BUILT_OTHER=signal_sh$(DOTEXE)

PPR_LIBS=PPOP.pm

#=== Build ==================================================================

all: $(BUILT_BINS) $(BUILT_LIBS) $(UNBUILT_LIBS) $(PPR_LIBS)

# A program which calls alert().
OBJS1=alert.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
alert$(DOTEXE): $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(INTLLIBS)

# Program to get a value from ppr.conf.
OBJS2=ppr_conf_query.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
ppr_conf_query$(DOTEXE): $(OBJS2)
	$(LD) $(LDFLAGS) -o $@ $(OBJS2) $(INTLLIBS)

# Program which prints PPR status message stream on stdout.
OBJS3=tail_status.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
tail_status$(DOTEXE): $(OBJS3)
	$(LD) $(LDFLAGS) -o $@ $(OBJS3) $(INTLLIBS)

# Wrapper for getsockbyname().
OBJS4=getservbyname.$(OBJ)
getservbyname$(DOTEXE): $(OBJS4)
	$(LD) $(LDFLAGS) -o $@ $(OBJS4) $(SOCKLIBS)

# A wrapper for mkstemp().
OBJS5=mkstemp.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
mkstemp$(DOTEXE): $(OBJS5)
	$(LD) $(LDFLAGS) -o $@ $(OBJS5)

# Signal values in Borne shell script form.
signal.sh: signal_sh$(DOTEXE)
	./signal_sh >signal.sh

# Shell version of respond.h.
respond.sh: ../include/respond.h
	echo "# Bourne shell translation of respond.h" >respond.sh
	sed -n -e 's/^#define \([^ ]*\) \([^ 	]*\)\([	]*\)\(.*\)$$/\1=\2\3#\4/p' \
		<../include/respond.h >>respond.sh

# Perl version of respond.h.
respond.ph: ../include/respond.h
	echo "# Perl translation of respond.h" >respond.ph
	sed -n -e 's/^#define \([^ ]*\) \([^ 	]*\)\([ 	]*\)\(.*\)$$/$$\1=\2;\3#\4/p' \
		<../include/respond.h >>respond.ph

# Shell version of commentary.h.
commentary.sh: ../include/commentary.h
	echo "# Bourne shell translation of commentary.h" >commentary.sh
	sed -n -e 's/^#define \([^ ]*\) \([^ 	]*\)\([	]*\)\(.*\)$$/\1=\2\3#\4/p' \
		<../include/commentary.h >>commentary.sh

# Perl version of commentary.h.
commentary.ph: ../include/commentary.h
	echo "# Perl translation of commentary.h" >commentary.ph
	sed -n -e 's/^#define \([^ ]*\) \([^ 	]*\)\([ 	]*\)\(.*\)$$/$$\1=\2;\3#\4/p' \
		<../include/commentary.h >>commentary.ph

#=== Install ================================================================

install: all
	$(INSTALLDATA) $(HOMEDIR)/lib $(BUILT_LIBS) $(UNBUILT_LIBS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib $(BUILT_BINS)
	$(MAKE_NEW_DIR) $(HOMEDIR)/lib/PPR
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib/PPR $(PPR_LIBS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(BUILT_BINS) $(BUILT_LIBS) $(BUILT_OTHER)

# end of file

