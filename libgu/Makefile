#
# mouse:~ppr/src/libgu/Makefile
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
# Last modified 9 September 2005.
#

include ../Makefile.conf

#=== Inventory ==============================================================

LIBOBJS=malloc.$(OBJ) \
	ini_section.$(OBJ) \
	getline.$(OBJ) \
	gu_md5.$(OBJ) \
	cloexec.$(OBJ) \
	gu_nonblock.$(OBJ) \
	daemon.$(OBJ) \
	gu_strcasecmp.$(OBJ) \
	gu_isdigit.$(OBJ) \
	gu_isxdigit.$(OBJ) \
	gu_isspace.$(OBJ) \
	gu_sscanf.$(OBJ) \
	gu_fgetint.$(OBJ) \
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
	gu_pch.$(OBJ) \
	gu_pca.$(OBJ) \
	gu_pcre_match.$(OBJ) \
	gu_pcre_split.$(OBJ) \
	gu_pca_join.$(OBJ) \
	gu_run.$(OBJ) \
	gu_exceptions.$(OBJ) \
	gu_uri.$(OBJ) \
	gu_strlower.$(OBJ) \
	gu_strtrim.$(OBJ) \
	pcre.$(OBJ) \
	pcre_study.$(OBJ) \
	gu_name_value.$(OBJ) \
	gu_utf8_decode.$(OBJ) \
	gu_utf8_printf.$(OBJ)

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
