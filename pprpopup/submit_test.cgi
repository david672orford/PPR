#! /usr/bin/perl

print "Content-Type: text/html\n";
print "\n";

print "<h1>CGI test</h1>\n";

print "<h2>Environment Variables</h2>\n";
print "<pre>\n";
foreach my $item (keys %ENV)
    {
    print "\$ENV{$item} = \"$ENV{$item}\"\n";
    }
print "</pre>\n";

print "<h2>Query String</h2>\n";
print "<pre>\n";
if($ENV{QUERY_STRING} ne "")
    {
    foreach my $item (split(/[&;]/, $ENV{QUERY_STRING}))
	{
	$item =~ s/\+/ /g;
	$item =~ s/%([0-9A-Fa-f]{2})/sprintf("%c",hex($1))/ge;
	print "$item\n";
	}
    }
print "</pre>\n";

print "<h2>POST data</h2>\n";
print "<pre>\n";
if($ENV{REQUEST_METHOD} eq "POST")
    {
    my $raw_data;
    read(STDIN, $raw_data, $ENV{CONTENT_LENGTH});
    foreach my $item (split(/[&;]/, $raw_data))
	{
	$item =~ s/\+/ /g;
	$item =~ s/%([0-9A-Fa-f]{2})/sprintf("%c",hex($1))/ge;
	print "$item\n";
	}
    }
print "</pre>\n";

exit 0;
