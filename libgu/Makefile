#
# mouse:~ppr/src/libgu/Makefile
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
# Last modified 23 January 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

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
	gu_snmp.$(OBJ) \
	gu_timeval.$(OBJ) \
	gu_pcs.$(OBJ) gu_pch.$(OBJ)

TARGETS=../libgu.$(LIBEXT)

#=== Build ==================================================================

all: $(TARGETS)

../libgu.$(LIBEXT): $(LIBOBJS)
	$(RMF) ../libgu.$(LIBEXT)
	$(LIBCMD) ../libgu.$(LIBEXT) $(LIBOBJS)
	$(RANLIB) ../libgu.$(LIBEXT)

#=== Install ================================================================

install: all

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(TARGETS)

# end of file

