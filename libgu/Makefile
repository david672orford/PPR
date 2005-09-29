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

LIBOBJS=malloc.o \
	ini_section.o \
	getline.o \
	gu_md5.o \
	cloexec.o \
	gu_nonblock.o \
	daemon.o \
	gu_strcasecmp.o \
	gu_isdigit.o \
	gu_isxdigit.o \
	gu_isspace.o \
	gu_sscanf.o \
	gu_fgetint.o \
	signal1.o \
	signal2.o \
	gu_snprintf.o \
	gu_strlcpy.o \
	gu_strlcat.o \
	torf.o \
	gu_strsignal.o \
	gu_strerror.o \
	getopt.o \
	lock_exclusive.o \
	padded.o \
	gu_strsep.o gu_strsep2.o \
	gu_wildmat.o \
	disk_space.o \
	gu_mkstemp.o \
	wordwrap.o \
	gu_getdouble.o \
	gu_dtostr.o \
	gu_psprintf.o \
	gu_snprintfcat.o \
	gu_snmp.o \
	gu_timeval.o \
	gu_pcs.o \
	gu_pch.o \
	gu_pca.o \
	gu_pcre_match.o \
	gu_pcre_split.o \
	gu_pca_join.o \
	gu_run.o \
	gu_exceptions.o \
	gu_uri.o \
	gu_strlower.o \
	gu_strtrim.o \
	pcre.o \
	pcre_study.o \
	gu_name_value.o \
	gu_utf8_decode.o \
	gu_utf8_printf.o \
	gu_utf8_put.o \
	gu_locale.o

TARGETS=../libgu.a

#=== Build ==================================================================

all: $(TARGETS)

../libgu.a: $(LIBOBJS)
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
	$(RMF) $(BACKUPS) *.o $(TARGETS)

# end of file
