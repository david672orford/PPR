#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/prn_addwiz.cgi.perl
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 26 May 2004.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_wizard.pl';
require 'cgi_intl.pl';

defined($CONFDIR) || die;
defined($PPOP_PATH) || die;
defined($PPAD_PATH) || die;
defined($PPR2SAMBA_PATH) || die;

# Where are our images?
my $IMAGES = "../images";

#===========================================
# This is the table for this wizard:
#===========================================
my $addgrp_wizard_table = [
		#===========================================
		# Welcome screen and name the group
		#===========================================
		{
		'title' => N_("Add a Group"),
		'picture' => "wiz-newgrp.jpg",
		'dopage' => sub {
				print "<P><span class=\"label\">", H_("PPR Group Queue Creation"), "</span></P>\n";

				print "<P>", H_("This program will guide you through the process of\n"
						. "setting up a group of printers in PPR."), "</P>\n";

				print "<p>", H_("The group must have a name.  The name may be up\n"
						. "to 16 characters long.  Control codes, tildes, and spaces\n"
						. "are not allowed.  Also, the first character may not be\n"
						. "a period or a hyphen."), "</p>\n";

				print "<p><span class=\"label\">", H_("Group Name:"), "</span><br>\n";
				print "<input tabindex=1 TYPE=\"text\" SIZE=16 NAME=\"name\" VALUE=", html_value(cgi_data_move('name', '')), ">\n";
				print "</p>\n";
				},
		'onnext' => sub {
				if($data{name} eq '')
					{ return _("You must enter a name for the group!") }
				if(-f "$CONFDIR/groups/$data{name}")
					{ return sprintf(_("The group \"%s\" already exists!"), $data{name}) }
				return undef;
				},
		'buttons' => [N_("_Cancel"), N_("_Next")]
		},

		#===========================================
		# Members list
		#===========================================
		{
		'title' => N_("Assign Group Members"),
		'picture' => "wiz-membership.jpg",
		'valign' => 'top',
		'dopage' => sub {
				require 'cgi_membership.pl';

				# For first time:
				if(!defined($data{nonmembers}))
					{
					require 'cgi_run.pl';

					# Members list starts empty.
					$data{members} = '';

					# Non-Members list starts as all printers.
					opencmd(PPOP, $PPOP_PATH, "-M", "dest", "all") || die;
					my @nonmembers = ();
					while(<PPOP>)
						{
						my($queue, $type) = split(/\t/, $_);
						next if($type ne 'printer');
						push(@nonmembers, $queue);
						}
					close(PPOP) || die;
					$data{nonmembers} = join(' ', sort(@nonmembers));
					}

				# Do double select box thing.
				&cgi_membership(_("Members:"), "members", 10, _("Non-Members:"), "nonmembers", 15);
				},
		'onnext' => sub {
				if($data{members} eq '')
					{ return _("The group must have at least one member!") }
				return undef;
				}
		},

		#===========================================
		# Assign a comment to the group
		#===========================================
		{
		'title' => N_("Describe the Group"),
		'picture' => "wiz-name.jpg",
		'dopage' => sub {
				print "<p>", html(sprintf(_("While short group names are convenient in certain contexts,\n"
						. "it is sometimes helpful to have a longer, more informative\n"
						. "description.  Please supply a longer description for\n"
						. "\"%s\" below."), $data{name})), "\n";
				print "</p>\n";

				print "<p><span class=\"label\">", H_("Description:"), "</span> ";
				print "<input tabindex=1 TYPE=\"text\" SIZE=40 NAME=\"comment\" VALUE=", html_value(cgi_data_move('comment', '')), ">\n";
				print "</p>\n";
				},
		'onnext' => sub {
				if($data{comment} eq '')
					 { return _("You must supply a description!") }
				return undef;
				},
		'buttons' => [N_("_Cancel"), N_("_Back"), N_("_Finish")]
		},

		#===========================================
		# Save the new group
		#===========================================
		{
		'title' => N_("Save the Group"),
		'picture' => "wiz-save.jpg",
		'dopage' => sub {
				require 'cgi_run.pl';

				print "<P><span class=\"label\">", H_("Saving new group:"), "</span></P>\n";
				print "<pre>\n";
				run("id");
				my @PPAD = ($PPAD_PATH, "--su", $ENV{REMOTE_USER});
				my $name = cgi_data_peek("name", "?");
				my $e = 0;
				$e || ($e=run(@PPAD, 'group', 'members', $name, split(/ /, $data{members})));
				$e || ($e=run(@PPAD, 'group', 'comment', $name, $data{comment}));
				$e || ($e=run($PPR2SAMBA_PATH, '--nocreate'));
				print "</pre>\n";

				if($e == 0)
					{
					print "<p>", H_("The queue has been created."), "</p>\n";
					}
				else
					{
					print "<p>", H_("Due to the problem indicated above, the queue was not created."), "</p>\n";
					}

				# Make the show_queues.cgi screen reload.  We really should
				# try to make sure there is one first!
				print "<script>window.opener.gentle_reload()</script>\n";
				},
		'buttons' => [N_("_Close")]
		}
];

#===========================================
# Main
#===========================================

&cgi_read_data();

&do_wizard($addgrp_wizard_table,
		{
		'auth' => 1,
		'imgdir' => "../images/"
		});

exit 0;

# end of file

