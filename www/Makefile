#
# mouse:~ppr/src/www/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 20 November 2002.
#

include ../Makefile.conf

#== Install Locations ========================================================

# Programs not invoked from the WWW interface.
BIN_DIR=$(HOMEDIR)/bin

# Library files and programs such as the cgi_*.pl libraries and the web server.
LIB_DIR=$(HOMEDIR)/lib

# Images that are part of the GUI.
IMAGES_DIR=$(WWWDIR)/images

# More images, pictures of printers an groups.
Q_ICONS_DIR=$(WWWDIR)/q_icons

# Javascript for the GUI.
JS_DIR=$(WWWDIR)/js

# Style sheets for the GUI.
STYLE_DIR=$(WWWDIR)/style

# HTML files for the GUI.
WWWDIR_GUI=$(WWWDIR)/html

# Last Queue Icon to be build.
Q_ICONS_LAST=q_icons/11110.png

# Where to HTML help files go?
HELPDIR=$(WWWDIR)/help

#== Inventory ================================================================

# These programs go into PPR's bin directory.
BIN_PROGS=ppr-passwd ppr-web

# These programs go into PPR's lib directory.
LIB_PROGS=ppr-httpd ppr-push-httpd$(DOTEXE)

# These programs go into cgi-bin.
CGI_PROGS=show_queues.cgi show_queues_nojs.cgi \
	prn_addwiz.cgi prn_properties.cgi prn_control.cgi prn_media.cgi prn_testpage.cgi \
	grp_addwiz.cgi grp_properties.cgi grp_control.cgi \
	alias_addwiz.cgi alias_properties.cgi \
	delete_queue.cgi \
	show_jobs.cgi job_modify.cgi job_log.cgi job_public.cgi scoreboard.cgi \
	commentary_select.cgi commentary_speach.cgi \
	responder_speach.cgi \
	test_cgi.cgi test_components.cgi test_tail_status.cgi \
	df_html.cgi \
	login_cookie.cgi \
	show_log.cgi show_printlog.cgi \
	docs_cat.cgi docs_man.cgi docs_plain.cgi docs_info.cgi docs_rpm.cgi docs_rpms_list.cgi \
	pprpopup_register.cgi \
	cliconf.cgi \
	window_open.cgi

LIBS=cgi_data.pl qquery_xlate.pl cgi_wizard.pl cgi_run.pl cgi_tabbed.pl cgi_error.pl \
	cgi_membership.pl cgi_time.pl cgi_auth.pl cgi_intl.pl cgi_back.pl cgi_redirect.pl \
	cgi_widgets.pl cgi_digest.pl \
	docs_util.pl ppd_select.pl

JAVASCRIPT=show_queues.js show_jobs.js md5.js

HTML_TOP=index.html robots.txt

HTML_GUI=robots.txt \
	commentary_select.html \
	test.html test-audio-commentary.html test-audio-responder.html test-cgi.html \
	show_logs.html \
	login_cookie.html login_cookie_frame2.html

HTML_HELP_EN=prn_properties.en.html show_jobs.en.html

HTML_HELP_RU=prn_properties.ru.html show_jobs.ru.html

HTML_HELP=$(HTML_HELP_EN) $(HTML_HELP_RU)

STYLES=show_queues.css show_jobs.css cgi_wizard.css cgi_tabbed.css \
	prn_control.css prn_media.css

#== Build ====================================================================

all: $(BIN_PROGS) $(LIB_PROGS) $(CGI_PROGS) $(LIBS) $(JAVASCRIPT) $(STYLES) $(HTML_TOP) $(HTML_GUI) $(Q_ICONS_LAST) images_subdir

OBJS=\
	ppr-push-httpd.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT)
ppr-push-httpd$(DOTEXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(Q_ICONS_LAST):
	( cd q_icons; ./make_composit_figs.perl )

images_subdir:
	( cd images && $(MAKE) )

#== Install ==================================================================

install: install-progs install-libs install-styles install-html install-images

install-progs: $(BIN_PROGS) $(LIB_PROGS) $(CGI_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(BIN_DIR) $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIB_DIR) $(LIB_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(CGI_BIN) $(CGI_PROGS)

install-libs: $(LIBS) $(JAVASCRIPT)
	$(MAKE_NEW_DIR) $(JS_DIR)
	$(INSTALLDATA) $(JS_DIR) $(JAVASCRIPT)
	$(INSTALLDATA) $(LIB_DIR) $(LIBS)

install-styles: $(STYLES)
	$(MAKE_NEW_DIR) $(STYLE_DIR)
	$(INSTALLDATA) $(STYLE_DIR) $(STYLES)

install-html: $(HTML_TOP) $(HTML_GUI) $(HTML_HELP)
	$(INSTALLDATA) $(WWWDIR) $(HTML_TOP)
	$(MAKE_NEW_DIR) $(WWWDIR_GUI)
	$(INSTALLDATA) $(WWWDIR_GUI) $(HTML_GUI)
	$(MAKE_NEW_DIR) $(HELPDIR)
	$(INSTALLDATA) $(HELPDIR) $(HTML_HELP)

install-images: images_subdir $(Q_ICONS_LAST)
	$(MAKE_NEW_DIR) $(IMAGES_DIR)
	$(INSTALLDATA) $(IMAGES_DIR) images/*
	$(MAKE_NEW_DIR) $(Q_ICONS_DIR)
	$(INSTALLDATA) $(Q_ICONS_DIR) q_icons/*.png

#== Housekeeping =============================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(BIN_PROGS) $(LIB_PROGS) $(CGI_PROGS)
	( cd q_icons; $(RMF) $(BACKUPS) )
	( cd images; $(MAKE) clean )

veryclean: clean
	$(RMF) q_icons/*.png

# end of file

