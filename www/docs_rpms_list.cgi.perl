#! @PERL_PATH@ -wT
#
# mouse:~ppr/src/www/docs_rpms_list.cgi.perl
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
# Last modified 13 January 2005.
#

use lib "@PERL_LIBDIR@";
use PPR;
require "cgi_data.pl";
require "cgi_intl.pl";

# Without this, rpm claims that it can't expand "~/.rpmrc".
$ENV{HOME} = ".";

# Without this Perl won't run rpm since the values are tainted.
$ENV{PATH} = $PPR::SAFE_PATH;
delete $ENV{ENV};

print <<"EndHead";
Content-Type: text/html

<html>
<head>
<title>RPM Packages Documentation Browser</title>
</head>
<body>
<h1>RPM Packages Documentation Browser</h2>
EndHead

eval {

my %groups = ();

open(RPM, "rpm -qa --queryformat \"%{GROUP}\\t%{NAME}-%{VERSION}\\t%{SUMMARY}\\n\" |") || die "open() failed, $!";
while(<RPM>)
	{
	my($group, $package, $summary) = split(/\t/);
	if(!exists $groups{$group})
		{
		$groups{$group} = [];
		}
	push(@{$groups{$group}}, [$package, $summary]);
	}
close(RPM) || die "close() failed, \$! = $!, \$? = $?";

foreach my $group (sort(keys %groups))
	{
	my $gp = $groups{$group};
	print "<h2>", html($group), "</h2>\n";
	print "<table border=1>\n";
	foreach $pp (@{$gp})
		{
		my($package, $description) = @{$pp};
		print "<tr>\n";
		print " <td><a href=\"docs_rpm.cgi?", form_urlencoded("pkg", $package), "\">", html($package), "</td>\n";
		print " <td>", html($description), "</td>\n";
		print "</tr>\n";
		}
	print "</table>\n";
	}

}; if($@)
	{
	my $error = $@;
	print "<p>", html($error), "\n";
	}

print <<"EndTail";
</body>
</html>
EndTail

exit 0;

