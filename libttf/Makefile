#
# mouse:~ppr/src/libttf/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 10 November 1998.
#

include ../Makefile.conf

OBJS=ttf_object.$(OBJ) endian.$(OBJ) fixed.$(OBJ) loadtable.$(OBJ) \
	loadname.$(OBJ) alloc.$(OBJ) \
	head.$(OBJ) name.$(OBJ) ps_header.$(OBJ) ps_trailer.$(OBJ) \
	charstrings.$(OBJ) ttf_get_psname.$(OBJ) \
	ps_type3.$(OBJ) ttf_psout.$(OBJ) ps_sfnts.$(OBJ) \
	ttf_strerror.$(OBJ)

all: ../libttf.$(LIBEXT)

install: ../libttf.$(LIBEXT)

libttf_private.h: ../include/libttf.h
	touch libttf_private.h

include .depend

../libttf.$(LIBEXT): $(OBJS)
	$(RMF) ../libttf.$(LIBEXT)
	-$(LIBCMD) ../libttf.$(LIBEXT) >/dev/null 2>&1
	$(LIBCMD) ../libttf.$(LIBEXT) $(OBJS)

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) ../libttf.$(LIBEXT)

# end of file



