#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/alias_properties.cgi.perl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 19 April 2002.
#

use lib "?";
require 'paths.ph';
require 'cgi_data.pl';
require 'cgi_tabbed.pl';
require 'cgi_intl.pl';
require 'cgi_widgets.pl';

defined($PPOP_PATH) || die;
defined($PPAD_PATH) || die;

my $tabbed_table = [
	#====================================================
	# Pane for the alias comment
	#====================================================
	{
	'tabname' => N_("Comment"),
	'dopage' => sub {
		my $comment = cgi_data_move('comment', '');

		print "<p><span class=\"label\">", H_("Alias Name:"), "</span> <span class=\"value\">", html($data{name}), "</span>\n";
		print "</p>\n";

		print "<p><span class=\"label\">", H_("Comment:"), "</span><br>\n";
		print "<input name=\"comment\" size=40 value=", html_value($comment), ">\n";
		print "</p>\n";
		}
	},

	#====================================================
	# Alias for what?
	#====================================================
	{
	'tabname' => N_("For What"),
	'cellpadding' => 20,
	'dopage' => sub {
		my $forwhat = cgi_data_move("forwhat", "");

		require 'cgi_run.pl';
		my @dest_list = ();
		opencmd(PPOP, $PPOP_PATH, "-M", "dest", "all") || die;
		while(<PPOP>)
		    {
		    my($queue, $type) = split(/\t/, $_);
		    if($type ne "alias")
			{
			push(@dest_list, $queue);
			}
		    }
		close(PPOP) || die;

		print "<p><span class=\"label\">\n";
		print html(sprintf(_("Printer or group for which \"%s\" is an alias:"), $data{name}));
		print "</span><br>\n";
		print "<select tabindex=1 name=\"forwhat\" size=12>\n";
		foreach my $dest (@dest_list)
		    {
		    print "<option value=\"$dest\"";
		    print " selected" if($dest eq $forwhat);
		    print ">$dest\n";
		    }
		print "</select>\n";
		print "</p>\n";
		}
	},

	#====================================================
	# Switchset
	#====================================================
	{
	'tabname' => N_("Switchset"),
	'dopage' => sub {
		print "<p><span class=\"label\">", H_("Switchset:"), "</span><br>\n";
		print "<textarea name=switchset cols=50 rows=10>\n";
		print &html(&cgi_data_move('switchset', ''));
		print "</textarea>\n";
		print "</p>\n";
		}
	},

	#====================================================
	# Samba
	#====================================================
	{
	'tabname' => N_("Samba"),
	'dopage' => sub {
		print "<div class=\"section\">\n";
		print "<span class=\"section\">";
		labeled_checkbox("addon ppr2samba", _("Share with Samba (provided Samba is prepared)"), 1, cgi_data_move("addon ppr2samba", 1));
		print "</span>\n";

		print "<p>";
		labeled_select("addon ppr2samba-prototype", _("Prototype Share:"),
			"", cgi_data_move("addon ppr2samba-prototype", ""), 
			"", "pprproto", "pprproto_pprpopup", "pprproto_pprpopup2");
		print "</p>\n";

		print "<p>";
		labeled_entry("addon ppr2samba-drivername", _("Override Win95 driver name:"), cgi_data_move("addon ppr2samba-drivername", ""), 20);
		print "</p>\n";

		print "<p>";
		labeled_select("addon ppr2samba-vserver", _("Assign to virtual server (Samba setup required):"),
			"", cgi_data_move("addon ppr2samba-vserver", ""),
			"", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10");
		print "</p>\n";

		print "</div>\n";

		print "<p>", _("Note that if the Samba configuration file smb.conf has not been edited as described in\n"
				. "the ppr2samba(8) manpage, the settings on this screen will have no effect."), "</p>\n";

		},
	'onleave' => sub {
		# This gives it a value 0 if it wasn't checked and blank if it was.
		$data{"addon ppr2samba"} = (cgi_data_move("addon ppr2samba", 0) ? "" : "0");
		return undef;
		}
	},

	#====================================================
	# Other settings
	#====================================================
	{
	'tabname' => N_("Other"),
	'dopage' => sub {
		print "<p><span class=\"label\">", H_("Passthru Printer Languages:"), "</span>\n";
		print "<input type=text size=16 name=\"passthru\" value=", html_value(cgi_data_move('passthru', '')), ">\n";
		print "</p>\n";
		}
	}
];

#============================================
# This function is called from do_tabbed().
# It uses the "ppad alias show" command to
# get the current alias configuration.
#============================================
sub load
{
require 'cgi_run.pl';

my $name = $data{name};
if(!defined($name)) { $name = '???' }

# Use "ppad -M show" to dump the alias's
# current configuration.
opencmd(PPAD, $PPAD_PATH, '-M', 'alias', 'show', $name) || die;
while(<PPAD>)
    {
    chomp;
    my($key, $value) = split(/\t/);
    $data{$key} = $value;		# copy to modify
    $data{"_$key"} = $value;		# copy to keep
    }
close(PPAD);

# Split the switchset into lines.
$data{switchset} =~ s/ -/\n-/g;
}

#============================================
# This function is called from do_tabbed().
# It uses the ppad command to save the
# changes to the printer configuration.
#============================================
sub save
{
require 'cgi_run.pl';
defined($PPAD_PATH) || die;
defined($PPR2SAMBA_PATH) || die;

my $name = $data{name};
my @PPAD = ($PPAD_PATH, "--su", $ENV{REMOTE_USER});
my $i;

print "<p><b>", H_("Saving changes:"), "</b></p>\n";
print "<pre>\n";

foreach $i (qw(forwhat comment passthru))
    {
    if($data{$i} ne $data{"_$i"})
	{ run(@PPAD, "alias", $i, $name, $data{$i}) }
    }

# Switchsets are very hard to do.
{
my $unwrapped_switchset = $data{switchset};
$unwrapped_switchset =~ s/\s*[\r\n]+\s*/ /g;
if($unwrapped_switchset ne $data{"_switchset"})
    {
    run(@PPAD, "alias", "switchset", $name, &shell_parse($unwrapped_switchset));
    }
}

# Do all of the "addon" stuff.
my $samba_addon_changed = 0;
foreach my $item (keys %data)
    {
    if($item =~ /^addon (.+)$/)
    	{
	my $key = $1;
	if($data{$item} ne $data{"_$item"})
	    {
	    $samba_addon_changed = 1 if($key =~ /^ppr2samba/);
	    run(@PPAD, "alias", "addon", $name, $key, $data{$item});
	    }
    	}
    }

# If anything that Samba cares about has changed, update it.
if($data{comment} ne $data{_comment} || $data{ppd} ne $data{_ppd})
    {
    run($PPR2SAMBA_PATH, '--nocreate');
    }

print "</pre>\n";

# Make the display queues screen reload.  We really should
# try to make sure there is one first!
print "<script>window.opener.gentle_reload()</script>\n";
}

#========================================
# main
#========================================

($<,$>) = ($>,$<);

&cgi_read_data();

&do_tabbed($tabbed_table, sprintf(_("PPR: Alias Properties: %s"), $data{name}), \&load, \&save, 9);

exit 0;

