#
# mouse:~ppr/src/www/cgi_back.pl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 14 June 2000.
#

#
# Attempt to set up the CGI variable "HIST" to store our PPR page navigation
# history.  We can do this if either HTTP_REFERER is defined or the caller
# provides a default URL to use if it is not defined.
#
sub cgi_back_possible
    {
    my $default = shift;

    if(!defined($data{HIST}))
	{
	if(defined($ENV{HTTP_REFERER}))
	    { $data{HIST} = $ENV{HTTP_REFERER} }
        elsif(defined($default))
	    { $data{HIST} = $default }
	}

    return defined($data{HIST});
    }

#
# Attempt to load the last URL listed in HIST into the browser.
#
sub cgi_back_doit
    {
    if(!defined($data{HIST}))
    	{
	require 'cgi_error.pl';
	error_doc("Can't Go Back", <<"EndOfText");;
<p>Attempt to return to previous screen failed because the URL of the previous
screen was lost.
</p>
EndOfText

	return;
    	}

    my @back_stack_list = split(/ /, $data{HIST});
    my $back_url = shift(@back_stack_list);
    if($back_url =~ /^\//)
    	{
	$back_url = "http://$ENV{SERVER_NAME}:$ENV{SERVER_PORT}$back_url";
    	}
    print STDERR "Going back to \"$back_url\"\n";

    my $html_body = <<"EndQuote10";
<html>
<head>
<title>Going Back</title>
<meta http-equiv="refresh" content="1; URL=$back_url">
</head>
<body>
<p>Going back to "<a href="$back_url">$back_url</a>"...
</body>
EndQuote10

    print <<"EndQuote20";
Content-Type: text/html
Content-Length: ${\length($html_body)}

EndQuote20

    print $html_body;
    }

#
# Return a new HIST encoded in QUERY_STRING form with the calling script as
# the last item.
#
sub cgi_back_stackme
    {
    my $hist = $data{HIST};
    my $myurl = $ENV{SCRIPT_NAME};
    if(defined($hist))
    	{
    	$hist .= " ";
    	$hist .= $myurl;
    	}
    else
    	{
    	$hist = $myurl;
    	}
    return form_urlencoded("HIST", $hist);
    }

1;

