#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/alias_addwiz.cgi.perl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 18 April 2002.
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
my $addalias_wizard_table = [
	#===========================================
	# Welcome screen and name the alias
	#===========================================
	{
	'title' => N_("Add an Alias"),
	'picture' => "wiz-newgrp.jpg",
	'dopage' => sub {
		print "<P><span class=\"label\">", H_("PPR Alias Creation"), "</span></P>\n";

		print "<P>", H_("This program will guide you through the process of\n"
			. "creating an alias for a printer or a group of printers."), "</P>\n";

		print "<p>", H_("The alias must have a name.  The name may be up\n"
			. "to 16 characters long.  Control codes, tildes, and spaces\n"
			. "are not allowed.  Also, the first character may not be\n"
			. "a period or a hyphen."), "</p>\n";

		print "<p><span class=\"label\">", H_("Group Name:"), "</span><br>\n";
		print "<input tabindex=1 TYPE=\"text\" SIZE=16 NAME=\"name\" VALUE=", html_value(cgi_data_move('name', '')), ">\n";
		print "</p>\n";
		},
	'onnext' => sub {
		if($data{name} eq '')
		    { return _("You must enter a name for the alias!") }
		if(-f "$CONFDIR/aliases/$data{name}")
		    { return sprintf(_("The alias \"%s\" already exists!"), $data{name}) }
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
	# Assign a comment to the alias
	#===========================================
	{
	'title' => N_("Describe the Alias"),
	'picture' => "wiz-name.jpg",
	'dopage' => sub {
		print "<p>", html(sprintf(_("While short group names are convient in certain contexts,\n"
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
	# Save the new alias
	#===========================================
	{
	'title' => N_("Save the Group"),
	'picture' => "wiz-save.jpg",
	'dopage' => sub {
		require 'cgi_run.pl';

		print "<P><span class=\"label\">", H_("Saving new alias:"), "</span></P>\n";
		print "<pre>\n";
		run("/usr/bin/id");
		my @PPAD = ($PPAD_PATH, "--su", $ENV{REMOTE_USER});
		run(@PPAD, 'alias', 'forwhat', $data{name}, split(/ /, $data{members}));
		run(@PPAD, 'alias', 'comment', $data{name}, $data{comment});
		run($PPR2SAMBA_PATH, '--nocreate');
		print "</pre>\n";

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
