#! /usr/bin/perl

use lib "?";
require "cgi_data.pl";
cgi_read_data();

my $name = cgi_data_move("name", "?");

print <<"EndOfHead";
Content-Type: text/html

<html>
<head>
<title>MCEC Public Printers</title>
<meta http-equiv="Refresh" content="15; url=$ENV{SCRIPT_NAME}?name=$name">
</head>
<body>
EndOfHead

# Start of exception handling block.
eval {

$name eq "mcec_las" || die;

$| = 1;
print "<h1>Printer Status</h1>\n<pre>\n";
$| = 0;

system("ppop status mcec_dup");

print "</pre>\n";

$| = 1;
print "<h1>Jobs Waiting</h1>\n<pre>\n";
$| = 0;

system("ppop --arrest-interest-interval 1200 list mcec_dup mcec_sim mcec_2up");

print "</pre>\n";

# This is the end of the exception handling block.  If die() was called
# within the block, print its message.
}; if($@)
    {
    my $message = html($@);
    print "<p>$message</p>\n";
    }

print <<"EndOfTail";
</body>
</html>
EndOfTail

exit 0;

