#
# mouse:~ppr/src/libppr/Makefile
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
# Last modified 23 March 2005.
#

include ../Makefile.conf

#=== Inventory ==============================================================

LIBOBJS=\
	int_main.$(OBJ) \
	int_prpapst.$(OBJ) int_cmdline.$(OBJ) \
	int_acache.$(OBJ) int_debug.$(OBJ) \
	int_copy_job.$(OBJ) int_tcp_connect.$(OBJ) \
	int_tcp_probe.$(OBJ) \
	datestamp.$(OBJ) \
	tokenize.$(OBJ) \
	qentryfile_clear.$(OBJ) qentryfile_load.$(OBJ) qentryfile_save.$(OBJ) qentryfile_free.$(OBJ) parse_qfname.$(OBJ) \
	quote.$(OBJ) \
	findres.$(OBJ) \
	protected.$(OBJ) money.$(OBJ) \
	charge.$(OBJ) jobid.$(OBJ) pagesize.$(OBJ) \
	options.$(OBJ) \
	dimens.$(OBJ) foptions.$(OBJ) ali_str.$(OBJ) \
	ppr_gcmd.$(OBJ) readppd.$(OBJ) \
	unsafe.$(OBJ) prune_env.$(OBJ) \
	acl.$(OBJ) \
	cs2enc.$(OBJ) enc2font.$(OBJ) font.$(OBJ) \
	ppr_fnamef.$(OBJ) \
	interfaces.$(OBJ) \
	alert.$(OBJ) \
	tail_status.$(OBJ) \
	query.$(OBJ) \
	queueinfo.$(OBJ) \
	pagemask.$(OBJ) \
	destinfo.$(OBJ) \
	ipp_utils.$(OBJ) \
	get_device_id.$(OBJ) \
	snmp_messages.$(OBJ) 

TARGETS=../libppr.$(LIBEXT) query_wrapper$(DOTEXE)

#=== Build ==================================================================

all: $(TARGETS)

# This build rule for this module is listed explicitly because of the
# -D option on in the cc command.
ali_str.$(OBJ): ali_str.c
	$(CC) $(CFLAGS) $(ATALKFLAGS) -DATALKTYPE_$(ATALKTYPE) -c ali_str.c

../libppr.$(LIBEXT): $(LIBOBJS)
	$(RMF) $@
	$(LIBCMD) $@ $^
	$(RANLIB) $@

query$(DOTEXE): query.c ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)
	$(CC) $(CFLAGS) -o $@ -DTEST $^

query_wrapper$(DOTEXE): query_wrapper.c ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)
	$(CC) $(CFLAGS) -o $@ -DTEST $^

#=== Install ================================================================

install: all
	$(INSTALLPROGS) root $(GROUP_PPR) 4755 $(LIBDIR) query_wrapper$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(TARGETS) query$(DOTEXE)

# end of file

