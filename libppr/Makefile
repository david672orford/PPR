#
# mouse:~ppr/src/libppr/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
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
# Last modified 30 March 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

LIBOBJS=\
	int_main.o \
	int_prpapst.o int_cmdline.o \
	int_acache.o int_debug.o \
	int_copy_job.o int_tcp_connect.o \
	int_tcp_probe.o \
	lpioc_get_device_id.o \
	datestamp.o \
	tokenize.o \
	qentryfile_clear.o qentryfile_load.o qentryfile_save.o qentryfile_free.o \
	parse_qfname.o \
	quote.o \
	findres.o \
	spool_state.o protected.o \
	money.o charge.o \
	jobid.o pagesize.o \
	options.o \
	dimens.o foptions.o ali_str.o \
	ppr_gcmd.o readppd.o \
	unsafe.o \
	renounce_root_privs.o \
	prune_env.o \
	acl.o \
	username_match.o \
	cs2enc.o enc2font.o font.o \
	ppr_fnamef.o \
	interfaces.o \
	alert.o \
	tail_status.o \
	query.o \
	queueinfo.o \
	pagemask.o \
	destinfo.o \
	ipp_obj.o ipp_req_attrs.o ipp_to_str.o ipp_str_to.o \
	snmp_messages.o \
	dispatch.o \
	pprd_call.o

TARGETS=../libppr.a query_wrapper$(DOTEXE)

#=== Build ==================================================================

all: $(TARGETS)

# This build rule for this module is listed explicitly because of the
# -D option on in the cc command.
ali_str.o: ali_str.c
	$(CC) $(CFLAGS) $(ATALKFLAGS) -DATALKTYPE_$(ATALKTYPE) -c ali_str.c

../libppr.a: $(LIBOBJS)
	$(RMF) $@
	$(LIBCMD) $@ $^
	$(RANLIB) $@

# This program tests the PostScript query class.
query$(DOTEXE): query.c ../libppr.a ../libgu.a
	$(CC) $(CFLAGS) -o $@ -DTEST $^

query_wrapper$(DOTEXE): query_wrapper.c ../libppr.a ../libgu.a
	$(CC) $(CFLAGS) -o $@ $^

#=== Install ================================================================

install: all
	$(INSTALLPROGS) root $(GROUP_PPR) 4755 $(LIBDIR) query_wrapper$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.o $(TARGETS) query$(DOTEXE)

# end of file

