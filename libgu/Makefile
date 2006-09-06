#
# mouse:~ppr/src/libgu/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 31 August 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

LIBOBJS=malloc.$(OBJ) malloc_debug.$(OBJ) \
	ini_section.$(OBJ) \
	getline.$(OBJ) \
	gu_md5.$(OBJ) \
	cloexec.$(OBJ) \
	gu_nonblock.$(OBJ) \
	daemon.$(OBJ) \
	gu_strcasecmp.$(OBJ) \
	gu_isdigit.$(OBJ) \
	gu_sscanf.$(OBJ) \
	gu_fscanf.$(OBJ) \
	signal1.$(OBJ) \
	signal2.$(OBJ) \
	gu_snprintf.$(OBJ) \
	gu_strlcpy.$(OBJ) \
	gu_strlcat.$(OBJ) \
	torf.$(OBJ) \
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
	gu_psprintf.$(OBJ) \
	gu_snprintfcat.$(OBJ) \
	gu_snmp.$(OBJ) \
	gu_timeval.$(OBJ) \
	gu_pcs.$(OBJ) \
	gu_run.$(OBJ) \
	gu_exceptions.$(OBJ) \
	pcre.$(OBJ) \
	pcre_get.$(OBJ) \
	pcre_maketables.$(OBJ) \
	pcre_study.$(OBJ) \
	c2lib_hash.$(OBJ) \
 	c2lib_pool.$(OBJ) \
	c2lib_pre.$(OBJ) \
	c2lib_pstring.$(OBJ) \
	c2lib_tree.$(OBJ) \
	c2lib_vector.$(OBJ)

TARGETS=../libgu.$(LIBEXT)

#=== Build ==================================================================

all: $(TARGETS)

../libgu.$(LIBEXT): $(LIBOBJS)
	$(RMF) $@
	$(LIBCMD) $@ $^
	$(RANLIB) $@

#=== Install ================================================================

install: all

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(TARGETS)

# end of file
