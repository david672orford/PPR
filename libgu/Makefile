#
# mouse:~ppr/src/libgu/Makefile
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
# Last modified 10 May 2001.
#

include ../makeprogs/global.mk

LIBOBJS=malloc.$(OBJ) malloc_debug.$(OBJ) \
	ini_section.$(OBJ) \
	getline.$(OBJ) \
	gu_throw.$(OBJ) \
	gu_md5.$(OBJ) \
	cloexec.$(OBJ) \
	gu_nonblock.$(OBJ) \
	daemon.$(OBJ) \
	gu_strcasecmp.$(OBJ) \
	gu_sscanf.$(OBJ) \
	gu_fscanf.$(OBJ) \
	signal1.$(OBJ) \
	signal2.$(OBJ) \
	snprintf.$(OBJ) \
	torf.$(OBJ) \
	gu_trim.$(OBJ) \
	gu_strsignal.$(OBJ) \
	gu_strerror.$(OBJ) \
	getopt.$(OBJ) \
	lock_exclusive.$(OBJ) \
	padded.$(OBJ) \
	gu_strsep.$(OBJ) gu_strsep2.$(OBJ) \
	gu_wildmat.$(OBJ) \
	disk_space.$(OBJ) \
	gu_mkstemp.$(OBJ) \
	wordwrap.$(OBJ) \
	gu_getdouble.$(OBJ) \
	gu_dtostr.$(OBJ) \
	gu_StrCopyMax.$(OBJ) \
	gu_StrAppendMax.$(OBJ) \
	gu_snprintfcat.$(OBJ) \
	gu_snmp.$(OBJ)

TARGETS=../libgu.$(LIBEXT)

all: $(TARGETS)

install: all

include .depend

../libgu.$(LIBEXT): $(LIBOBJS)
	$(RMF) ../libgu.$(LIBEXT)
	$(LIBCMD) ../libgu.$(LIBEXT) >/dev/null 2>&1
	$(LIBCMD) ../libgu.$(LIBEXT) $(LIBOBJS)

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(TARGETS)

# end of file

