#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/ppr-httpd.perl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 18 October 2001.
#

# Filled in by installscript:
use lib "?";
require "paths.ph";
require "cgi_time.pl";
use Socket;

# Suppress mistaken warnings about things defined in paths.ph.  Otherwise,
# if we use them only once, Perl with the -w switch complains that they
# have been refered to only once!
defined($HOMEDIR) || die;
defined($SHAREDIR) || die;
defined($LOGDIR) || die;
defined($SHORT_VERSION) || die;
defined($SAFE_PATH) || die;
defined($CONFDIR) || die;

# The text for "Server:" header and $ENV{SERVER_SOFTWARE}.  It is based on
# the PPR version number.
my $SERVER = "PPR-httpd/$SHORT_VERSION";

# The real directory which coresponse to the web server root directory.
my $WEBROOT = "$SHAREDIR/www";

# The real directory which coresponses to the virtual directory "/cgi-bin/".
my $CGI_BIN = "$HOMEDIR/cgi-bin";

# The time in seconds that we will wait for the next request on a
# persistent connexion.
my $PERSISTENT_TIMEOUT = 600;

# How long should we wait to receive the complete header?
my $HEADER_TIMEOUT = 60;

# Buffer for reading files for GET responses.
my $GET_FILEREAD_BUFSIZE = 16384;

# Settings for receiving POST data.
my $POST_REQUEST_BUFSIZE = 4096;
my $POST_REQUEST_TIMEOUT = 600;
my $POST_RESPONSE_BUFSIZE = 4096;

# Temporary
$PWFILE = "$CONFDIR/htpasswd";
$SECRET = "jlkfjasdf8923sdklf";
$REALM = "printing";
$MAX_NONCE_AGE = 600;

# One day expressed in seconds.
$DAY = (24 * 60 * 60);

# Regular expression definition of an HTTP token.
# The set of characters valid in a token (which the request method is)
# is defined in RFC 2068 section 2.2.  In the regular expression I have
# escaped everything not alpha-numberic just to be plain and to be on
# the safe side.
my $TOKEN = '[\!\#\$\%\&\x27\*\+\-\.0-9A-Z\^\_\`a-z\|\~]';

# HTTP error code description strings.
%RESPONSE_CODES = (
	200 => 'OK',
	201 => 'Created',
	202 => 'Accepted',
	203 => 'Non-Authoritative Information',
	204 => 'No Content',
	205 => 'Reset Content',
	206 => 'Partial Content',
	300 => 'Multiple Choices',
	301 => 'Moved Permanently',
	302 => 'Moved Temporarily',
	303 => 'See Other',
	304 => 'Not Modified',		# yet to be implemented
	305 => 'Use Proxy',
	400 => 'Bad Request',
	401 => 'Unauthorized',
	402 => 'Payment Required',
	403 => 'Forbidden',
	404 => 'Not Found',
	405 => 'Method Not Allowed',
	406 => 'Not Acceptable',
	411 => 'Length Required',
	416 => 'Requested Range Not Satisfiable',
	500 => 'Internal Server Error',
	501 => 'Not Implemented',
	505 => 'HTTP Version Not Supported'
);

#===========================================================
# Initialization code.
#===========================================================

# Ditch Linux environment variables which offend or sense of neatness because
# they are not meaningful (or even necessarily true) in the context of a CGI 
# script.
delete $ENV{CONSOLE};
delete $ENV{TERM};
delete $ENV{HOME};
delete $ENV{USER};
delete $ENV{LOGNAME};
delete $ENV{MAIL};
delete $ENV{_};
delete $ENV{DISPLAY};
delete $ENV{XAUTHORITY};
delete $ENV{PWD};
delete $ENV{OLDPWD};
delete $ENV{inetd_dummy};

# Ditch environment variables which try to set the language.  We will be using
# the language preferences of the web client rather than the language
# preferences of the person who started Inetd.
delete $ENV{LANG};
delete $ENV{LANGUAGE};		# GNU
delete $ENV{LC_MESSAGES};
delete $ENV{LC_ALL};
delete $ENV{LINGUAS};		# Who knows

# This is the PATH that we will feed to CGI scripts.
$ENV{PATH} = $SAFE_PATH;

# These variables specify files which sh, ksh, and bash should source during 
# startup.  Since taint checks are on, Perl won't do exec() if these
# are defined since they could alter the semantics of a shell script.
# or shell command.  The same goes for IFS since it changes how the shells
# parse commands.  I don't know what CDPATH is, but Perl 5.6.0 checks for it.
delete $ENV{ENV};
delete $ENV{BASH_ENV};
delete $ENV{IFS};
delete $ENV{CDPATH};

# Set the umask so that our log file will have the correct permissions.
# Remember that we will be running under the user "pprwww" and the
# group "ppr".
umask(002);

# If we have access, send STDERR to a debugging file, otherwise throw it
# away.  We must do something with it so it doesn't corrupt the HTTP
# transaction.
open(STDERR, ">>$LOGDIR/ppr-httpd") || open(STDERR, ">/dev/null") || die;

#===========================================================
# If in standalone mode, create the socket and wait for a connexion.
#===========================================================
my $standalone_port = undef;
if(scalar @ARGV >= 1 && $ARGV[0] =~ /^--standalone-port=(.+)$/)
    {
    $standalone_port = $1;
    }
elsif(scalar @ARGV >= 2 && $ARGV[0] eq "--standalone-port" && $ARGV[1] =~ /^(.+)$/)
    {
    $standalone_port = $1;
    }
if(defined $standalone_port)
    {
    if($standalone_port !~ /^\d+$/)
    	{
	($standalone_port) = (getservbyname($standalone_port, "tcp"))[2];
	defined($standalone_port) || die;
    	}

    socket(SERVER, PF_INET, SOCK_STREAM, getprotobyname("tcp")) || die $!;
    setsockopt(SERVER, Socket::SOL_SOCKET, Socket::SO_REUSEADDR, 1) || die $!;
    my $my_address = sockaddr_in($standalone_port, INADDR_ANY);
    bind(SERVER, $my_address) || die $!;
    listen(SERVER, SOMAXCONN) || die $!;

    $SIG{CHLD} = 'IGNORE';
    while(1)
	{
	accept(CLIENT, SERVER) || die $!;
	my $pid = fork();
	if(!defined $pid)
	    {
	    print STDERR "Fork failed, $!\n";
	    close CLIENT;
	    next;
	    }
	if($pid)
	    {
	    close CLIENT;
	    next;
	    }
	close SERVER;
	open(STDIN, "<&CLIENT") || die $!;
	open(STDOUT, ">&CLIENT") || die $!;
	last;
	}
    }

#===========================================================
# Start of connection handling code.
#===========================================================

# This is used to detect idle connexions.
$SIG{ALRM} = sub { die "alarm\n" };

# If STDIN is a socket, determine the IP address of the client.
{
my $remote_sockaddr = getpeername(STDIN);
if(defined($remote_sockaddr))
    {
    my $remote_addr;
    ($REMOTE_PORT, $remote_addr) = unpack_sockaddr_in($remote_sockaddr);
    $ENV{REMOTE_ADDR} = inet_ntoa($remote_addr);
    }
else
    {
    $ENV{REMOTE_ADDR} = "";
    $REMOTE_PORT = 0;
    }
}
print STDERR "Connect from \"$ENV{REMOTE_ADDR}:$REMOTE_PORT\".\n";

#===========================================================
# Main Request Loop
#
# When we break out of this loop, the server shuts down.
#===========================================================
while(1)
    {
    #
    # Read a request line.
    #
    my $request;
    eval
	{
	alarm($PERSISTENT_TIMEOUT);	# start timeout
	$request = <STDIN>;		# read request line
	alarm(0);			# cancel timeout
	};
    if($@)
	{
	die unless $@ eq "alarm\n";
	last;
	}

    # If 0 bytes were read,
    if(!defined($request))
    	{
	print STDERR "Client disconnected.\n";
	last;
    	}

    my $request_time = time();

    print STDERR "===  $$  ================================================\n";
    print STDERR $request;

    # I don't no if it is our fault or not, but Netscape 4.6 has been known
    # to send unexpected blank lines.
    if($request eq "\r\n")
    	{
	print STDERR "Not a request, just a spurious blank line.\n\n";
	next;
    	}

    # Start to construct the response headers.  We begin with the
    # easy stuff.
    my $resp_headers_general = '';
    $resp_headers_general .= "Server: $SERVER\r\n";
    $resp_headers_general .= ("Date: " . cgi_time_format($request_time) . "\r\n");

    #
    # Read the request header.
    #
    # Header names are case-insensitive.  Though esthetically pleasing,
    # a space after the colon is optional.  Line which begin with spaces
    # or tabs are continuation lines and are appended to the previous line.
    # If a header appears twice, the contents are concatenated with a
    # comma separating them.
    #
    my %request_headers = ();
    {
    my $prevname = "";		# last header (for header continuation)
    while(1)
        {
	eval
	    {
	    alarm($HEADER_TIMEOUT);
	    $_ = <STDIN>;
	    alarm(0);
	    };
	if($@)
	    {
	    die unless $@ eq "alarm\n";
	    die "Timeout after $HEADER_TIMEOUT seconds while reading header.\n";
	    }

	last if(! defined $_);

	print STDERR $_;
        last if(/^\r?\n$/);

	# If header line with keyword on left,
        if(/^([^:\s]+):\s*([^\r\n]+)/)
            {
	    my($name, $value) = ($1, $2);

	    # Convert to upper case and hyphens to underscores.
	    $name =~ tr/[a-z]-/[A-Z]_/;

	    # If there already was a header line of this type, combine
	    # values with a comma as specified by RFC 2068 section 4.2.
	    if(defined($request_headers{$name}))
		{ $request_headers{$name} .= ",$value" }

	    # Otherwise, just assign it.
	    else
	 	{ $request_headers{$name} = $value }

	    $prevname = $name;
            }

	# If header continuation line, add the previous value.
	# RFC 2068 does not mention inserting a comma in section 4.2.
	elsif(/^\s+([^\r\n]+)/)
	    {
	    $request_headers{$prevname} .= " $1";
	    }
    	} # line reader
    } # Read request headers

    # We will determine the correct value later.  We set it to "close" here
    # in case die is called in the following eval block before we can
    # determine what value is acceptable to the client.
    my $resp_header_connection = "close";

    #
    # Start an exception handling block.  If we die in this block
    # then code at the bottom of this block will generate an
    # appropriate HTTP response.  If the argument to die begins
    # with a 3 digit number then it will be the HTTP response code.
    # Anything after the code will be included in a document explaining
    # that an error has occured.
    #
    eval
	{
        #
        # Parse the request line.  A typical line is:
	#
        # GET /index.html HTTP/1.1
        #
	# The set of characters valid in a token (which the request method is)
	# is defined in RFC 2068 section 2.2.  In the regular expression I have
	# escaped everything not alpha-numberic just to be plain and to be on
	# the safe side.
        #
        if($request !~ /^($TOKEN+)\s+(\S+)\s+HTTP\/0*(\d+)\.0*(\d+)\s*$/o)
	    {
	    my $stript_request = $request;
	    $stript_request =~ s/[\r\n]+$//;
	    die "400 <hr><pre>$stript_request</pre><hr>The HTTP request line shown above contains a syntax error.\n";
	    }

	# Assign names to the parts we found during the parse.
	# The request method is case-sensitive per section
	# 5.1.1, so we won't be converting it to upper case.
        my($request_method, $request_uri, $request_version_major, $request_version_minor) = ($1,$2,$3,$4);

	# We do not support HTTP 0.9 or the as yet uninvented 2.0.
	if($request_version_major != 1)
	    {
	    die "505 This server cannot interpret HTTP version $request_version_major.$request_version_minor requests.\n";
	    }

        #
        # Based on the HTTP version and the "Connection:" header,
        # determine if persistent connections are in effect.
	#
	# This code has to come this late because it needs to test
	# the value of $request_version_minor.  Thus, if there is
	# a parse error in the request header, the connexion will
	# be closed after the response is sent.
        #

	# Default for HTTP 1.1 is not to close.
	$resp_header_connection = "";

	# Default for HTTP 1.0 is the oposite.
        if($request_version_minor < 1)
            { $resp_header_connection = "close" }

	#
	# Did the client ask for a specific setting?
	# Note that I cannot find anything in RFC 2068 to suggest that the
	# values of these tokens are case-insensitve.  I have noticed
	# that HotJava 3.0 send "Connection: keep-alive".  At the moment I
	# consider that a bug we won't accomodate since the failure is
	# graceful.
	#
        {
        my $c;
        if(defined($c = $request_headers{CONNECTION}))
            {
	    while($c =~ m/([^ \t,]+)[ \t,]*/g)
	    	{
		my $token = $1;
		print STDERR "Connection token: $token\n";

		# HTTP 1.1 persistent connection declined
		if($token eq "close")
                    { $resp_header_connection = "close" }

		# HTTP 1.0 persistent connnection requested
		elsif($token eq "Keep-Alive")
                    { $resp_header_connection = "Keep-Alive" }
		}
            }
	}

        #
        # Separate the request URL into host, path, and query.
        #
        my $path = $request_uri;
        if($path =~ /^http:\/\/([^\/]+)\/(.*)$/)
            {
            $request_headers{HOST} = $1;
            $path = $2;
            }
	my $query = "";
	if($path =~ /^([^\?]*)\?([^\?]*)$/)
	    {
	    $path = $1;
	    $query = $2;
	    }
        $path =~ s/%([0-9A-Z][0-9A-Z])/sprintf("%c",hex($1))/ige;
        die "400 The request URI \"$path\" does not begin with a slash.\n" unless($path =~ /^\//);

        #
        # Clean up the path to prevent security breaches.
        #
        print STDERR "Raw path: \"$path\"\n";
        {
        $path =~ s/^\///;
        my @path_out = ();
        my $element;
        foreach $element (split(/\//, $path, 1000))
            {
            next if($element eq '.');

            if($element eq '..')
                {
                if(@path_out < 1)
                    {
                    die "400 The path \"/$path\" is outside the server root.\n";
                    next;
                    }
                else
                    {
                    pop(@path_out);
                    }
                }
            else
                {
                push(@path_out, $element);
                }
            }
        $path = join('/', @path_out);
        }
        print STDERR "Cleaned path: \"$path\"\n";

	#
	# Make sure the request method is one we support on at least one URL.
	#
	if($request_method ne "GET" && $request_method ne "POST")
	    {
	    $resp_headers_general .= "Public: GET, POST\r\n";
	    die "501 The only request methods that this server can accept are GET and POST.\n";
	    }

	#
	# If the request is for a CGI script,
	#
	if($path =~ /^cgi-bin\/([^\/]+)(.*)$/)
	    {
	    if($request_method eq "GET" || $request_method eq "POST")
                {
                $resp_header_connection =
                    do_cgi($request_method, $request_uri, \%request_headers,
			"cgi-bin", $1, $2, $query,
                        $resp_header_connection, $resp_headers_general,
			$request_time,
			$request_version_major, $request_version_minor);
		}
	    else
	    	{
		$resp_headers_general .= "Allow: GET, POST\r\n";
		die "405 The only request methods that this server can accept for CGI scripts are GET and POST.\n";
	    	}
            }

	#
	# Is the request for ppr-push-httpd?
	#
	elsif($path =~ /^push\//)
	    {
	    print STDERR "Launching push server...\n";
	    do_push($path, $request_method, $request_uri, \%request_headers,
	    	$request_version_major, $request_version_minor);
	    print STDERR "Done, exiting.\n\n";
	    exit 0;
	    }

	#
	# If we get here the request is presumably for a real file.
	#
        else
            {
            if($request_method eq "GET")
                {
                do_get(\%request_headers, $path, $resp_header_connection, $resp_headers_general, $request_time);
		}
	    else
	    	{
		$resp_headers_general .= "Allow: GET\r\n";
	    	die "405 The only request method that this server can accept for files is GET.\n";
	    	}
            }
	};

    #
    # Catch exceptions and create a responses describing them.
    # The response is a valid HTTP 1.1 response with an HTML
    # entity body.
    #
    if($@)
	{
	my $message = $@;

	# If the error message begins with a 3 digit number, it is
	# the HTTP error code.  Otherwise, use an error code
	# of 500 (internal server error).
        my($code, $explanation);
        if($message =~ /^(\d\d\d)\s+(.*)/)
            {
            ($code, $explanation) = ($1, $2);
            }
        else
            {
            ($code, $explanation) = (500, $message);
            }

	# Get a human-readable description of the response code.
        my $code_description = $RESPONSE_CODES{$code};

	# We must construct the entity before sending the
	# header because we want to send a "Content-Length:"
	# header line.
	my $body = <<"EndExceptionEntity";
<html>
<head>
<title>$code $code_description</title>
</head>
<body>
<h1>$code $code_description</h2>
<p>$explanation</p>
<p>
<b>Server:</b>  $SERVER<br>
<b>Host:</b>  ${\`uname -n`}<br>
<b>Date:</b>  ${\cgi_time_format($request_time)}<br>
</p>
</body>
</html>
EndExceptionEntity

        print "HTTP/1.1 $code $code_description\r\n";
	print $resp_headers_general;
	if($resp_header_connection ne '')
	    { print "Connection: $resp_header_connection\r\n" }
        print "Content-Type: text/html\r\n";
        print "Content-Length: ", length($body), "\r\n";
        print "\r\n";
	print "$body";

	print STDERR "$code $explanation\n";
	}

    # Flush the output buffer so that the last of
    # the response is transmitted to the client.
    $| = 1; print ""; $| = 0;

    # If this is not to be a persistent connection,
    # close it now.
    last if($resp_header_connection eq 'close');

    print STDERR "Ready for next request.\n\n";
    }

print STDERR "Server shutdown.\n\n";
exit 0;

#=========================================================================
# This routine handles a GET request for a static file.  Its parameters
# are the request headers, the file's path with reference to the root of
# that virtual host, the desired content of the "Connection:" header
# (empty if none should be sent), sundry extra response headers, and the
# Unix format time at which the request was received.
#=========================================================================
sub do_get
    {
    my($request_headers, $path, $resp_header_connection, $resp_headers_general, $request_time) = @_;

    my $full_path = "$WEBROOT/$path";

    # If it ends in a slash, it is a directory request, so check for an
    # "index.html" file.  If it isn't found, then we can't fulfill
    # this request.
    if($path eq "" || $path =~ /\/$/)
        {
	$full_path .= "index.html";
	if(! -f $full_path)
	    {
	    die "501 Directory listings are not implemented by this server, and there is no index.html file.\n";
	    }
        }

    # Defaults:
    my $mime_type = "text/plain";
    my $max_lifetime = (7 * $DAY);

    # Long list of exceptions:
    if($full_path =~ /\.html?$/i)
	{
	$mime_type = "text/html;charset=iso-8859-1";
	}
    if($full_path =~ /\.txt$/i)
    	{
    	$mime_type = "text/plain";
    	}
    elsif($full_path =~ /\.jpe?g$/i)
    	{
    	$mime_type = "image/jpeg";
    	$max_lifetime = (30 * $DAY);
    	}
    elsif($full_path =~ /\.gif$/i)
	{
	$mime_type = "image/gif";
	$max_lifetime = (30 * $DAY);
	}
    elsif($full_path =~ /\.png$/i)
    	{
    	$mime_type = "image/png";
    	$max_lifetime = (30 * $DAY);
    	}
    elsif($full_path =~ /\.ps$/i)
    	{
    	$mime_type = "application/postscript";
    	}
    elsif($full_path =~ /\.pdf$/i)
    	{
    	$mime_type = "application/pdf";
    	}
    elsif($full_path =~ /\.js$/i)
    	{
    	$mime_type = "text/javascript";
    	}
    elsif($full_path =~ /\.css$/i)
    	{
    	$mime_type = "text/css";
    	}
    elsif($full_path =~ /\.img$/i)	# Our content negotionation
    	{
	die "501 .img content negotiation not yet implemented";
    	}
    elsif($full_path =~ /\.var$/i)	# Apache content negotiation
	{
	die "501 .var content negotiation not implemented";
	}

    if(!open(F, "<$full_path"))
	{
	my $error = $!;
	if($error =~ /^No such /i)
	    {
	    # Not Found
	    die "404 The file \"$path\" does not exist.\n";
	    }
	elsif($error =~ /denied/i)
	    {
	    # Forbidden
	    die "403 Access to the file \"$path\" is denied.\n";
	    }
	else
	    {
	    # Internal Server Error
	    die "Can't open \"$path\" for read.  Error: $!\n\n";
	    }
	}

    # We do not support automatic redirects if the user leaves off
    # the trailing slash on a directory URL.  We want the HTML
    # author to fix the broken link.
    if(-d F)
        {
        die "404 The file \"/$path\" is not found, but there is a directory \"/$path/\".\n";
        }

    # Extract the file size and modification time from the result of the
    # implicit fstat() performed by the -d operator above.
    my($size, $mtime) = (stat _)[7,9];
    print STDERR "File size is $size bytes, mtime is $mtime.\n";

    # Do expiration date calculation.  The date is placed in the future
    # by the same amount of time as the modification was in the past,
    # but, the time into the future it clipped to $max_lifetime.
    my $lifetime = ($request_time - $mtime);
    if($lifetime > $max_lifetime) { $lifetime = $max_lifetime }

    # Determine if "If-Modified-Since:" means we should not send
    # the entity body.
    my $status = 200;
    if(defined(my $ims = $request_headers->{IF_MODIFIED_SINCE}))
    	{
	# Netscape 4.7 adds this spurious information:
	$ims =~ s/; length=\d+//;

	my $ims_parsed;
	if(!defined($ims_parsed = cgi_time_parse($ims)))
	    {
	    print STDERR "Invalid \"If-Modified-Since: $ims\"\n";
	    }
	else
	    {
	    if($mtime <= $ims_parsed)
	    	{
		print STDERR "Not modified.\n";
	    	$status = 304;
	    	}
	    }
    	}

    # If a range conditions isn't met, forbid sending partial content.
    my $if_range_failed = 0;
    if(defined(my $rd = $request_headers->{IF_RANGE}))
	{
        my $rd_parsed;
        if(!defined($rd_parsed = cgi_time_parse($rd)))
            {
	    print STDERR "Unparsable \"If-Range: $rd\".\n";
            }
	else
	    {
	    if($mtime > $rd_parsed)
	    	{
		print STDERR "\"If-Range: $rd\" is false, \"Range:\" will be ignored.\n";
	    	$if_range_failed = 1;
	    	}
	    }
	}

    # Determine if we should send just a byte range.
    my $content_range = undef;
    if(!$if_range_failed && defined(my $range = $request_headers->{RANGE}))
	{
	if($range !~ /^bytes=(\d*)-(\d*)$/)
	    {
	    print STDERR "Unparsable or unsupported multiple \"Range: $range\".\n";
	    }
	else
	    {
	    my($start, $stop) = ($1, $2);
	    # Save the size before we mess with it.
	    my $total_entity_size = $size;
	    if($start ne "")		# if absolute range,
	    	{
		if($start >= $size)	# if beyond end of file,
		    {
		    $status = 416;	# requested range not satisfiable
		    $content_range = "bytes */$total_entity_size";
		    }
		else
		    {
                    $status = 206;	# partial content
                    if($start > 0)      # if sysseek() really necessary
                        {
                        sysseek(F, $start, 0) || die;
                        $size -= $start;
                        $size = 0 if($size < 0);
                        }
                    if($stop ne "")
                        {
                        my $new_size = ($stop - $start);
                        $size = ($new_size > $size) ? $size : $new_size;
                        }
		    $stop = ($start + $size);
                    $content_range = "bytes $start-$stop/$total_entity_size";
		    }
	    	}
	    elsif($stop ne "")		# end relative range
	    	{
		$status = 206;		# partial content
                $size = ($stop > $size) ? $size : $stop;
		$start = ($total_entity_size - $size);
		$stop = ($start + $size);
		sysseek(F, $start, 0) || die;
		$content_range = "bytes $start-$stop/$total_entity_size";
	    	}
	    else
	    	{
		print STDERR "Meaningless \"Range: $range\".\n";
	    	}
	    }
	}
    if(defined($content_range))
    	{ print STDERR "Sending only range \"$content_range\".\n" }

    print "HTTP/1.1 $status ", $RESPONSE_CODES{$status}, "\r\n";
    print $resp_headers_general;
    print "Connection: $resp_header_connection\r\n" if($resp_header_connection ne '');
    print "Expires: ", cgi_time_format($request_time + $lifetime), "\r\n";

    # RFC 2616 section 10.3.5 says we must not send these if we are
    # sending the 304 response code.
    if($status != 304)
	{
	print "Last-Modified: ", cgi_time_format($mtime), "\r\n";
	print "Content-Type: $mime_type\r\n";
	print "Content-Length: $size\r\n";
	print "Content-Range: $content_range\r\n" if(defined($content_range));
	}

    print "\r\n";

    # Do the file copying.  Notice that if the file has grown since
    # we did the stat(), the new part of the file will not be sent.
    # This is deliberate since we have already told the client
    # the content length.
    if($status != 304)
	{
        my $buffer;
        my $toread;
        my $gotten;
	while($size > 0)
	    {
	    $toread = ($size > $GET_FILEREAD_BUFSIZE) ? $GET_FILEREAD_BUFSIZE : $size;
            $gotten = sysread(F, $buffer, $toread);
	    if(!defined($gotten))
	    	{
		print STDERR "sysread() failed during GET, $!\n";
		exit 0;
	    	}
	    if($gotten < $toread)
	    	{
		print STDERR "File grew shorter by ", ($toread - $gotten), " bytes during GET!\n";
		exit 0;
	    	}
            print $buffer;
	    $size -= $gotten;
            }
	}

    if(!close(F))
    	{ print STDERR "close() failed after GET, $!\n" }

    print STDERR "Done sending file.\n\n";
    } # do_get()

#=========================================================================
# Fulfill a CGI request, either a GET or a POST.
#=========================================================================
sub do_cgi
    {
    my($method, $request_uri, $request_headers, $http_cgi_dir, $path, $path_info, $query, $resp_header_connection, $resp_headers_general, $request_time, $request_version_major, $request_version_minor) = @_;
    my $protection_domain = "http://$request_headers->{HOST}/$http_cgi_dir/";
    my $stale = 0;
    my $auth_info = undef;

    if(! -f "$CGI_BIN/$path")
	{ die("404 The CGI program \"$path\" is not found.\n") }
    if(! -x _)
    	{ die("403 The CGI program \"$path\" is not executable.\n") }

    $ENV{REMOTE_USER} = "";

    # Look for an "Authorization:" header with MD5 Digest credentials.  If
    # such a header is found, and the credentials are good, then this will
    # set the REMOTE_USER and set AUTH_TYPE to "Digest".
    ($ENV{AUTH_TYPE}, $ENV{REMOTE_USER}, $stale, $auth_info)
    	= auth_verify($method, $request_uri, $protection_domain, $request_headers->{AUTHORIZATION});

    # If that didn't set the username, try the cookie scheme.  If valid
    # credentials are found in a cookie, this will set REMOTE_USER
    # and set AUTH_TYPE to "Cookie".
    if($ENV{REMOTE_USER} eq "")
	{
	($ENV{AUTH_TYPE}, $ENV{REMOTE_USER})
	    = auth_verify_cookie($method, $request_uri, $protection_domain, $request_headers->{COOKIE});
	}

    # If that above didn't work, try Linux localhost authorization.  Note that
    # this test will effectively prevent MD5 Digest authentication from being
    # used on a localhost connexion since no "WWW-Authenticate:" line will
    # ever be sent.
    if($ENV{REMOTE_USER} eq "" && $ENV{REMOTE_ADDR} eq "127.0.0.1")
    	{
	($ENV{AUTH_TYPE}, $ENV{REMOTE_USER}) = auth_verify_localhost($REMOTE_PORT);
    	}

    # Some users consider Tcpwrappers security to be sufficient.  If you want
    # to live dangerously, uncomment this code and change "ppranon" to a an
    # appropriate username for anonymous access.
    #if($ENV{REMOTE_USER} eq "")
    #	{
    #	($ENV{AUTH_TYPE}, $ENV{REMOTE_USER}) = ("None", "ppranon");
    #	}

    print STDERR "Executing CGI program \"$CGI_BIN/$path\".\n";

    # Create two anonymous pipes, one to send data to the CGI script,
    # the other to receive data.
    pipe(QUERY_READ, QUERY_WRITE) || die "pipe() failed, $!";
    pipe(RESP_READ, RESP_WRITE) || die "pipe() failed, $!";

    my $pid = fork();
    defined($pid) || die "fork() failed, $!";

    # If child,
    if($pid == 0)
	{
	eval
	    {
	    # Close our copies of the parent ends of the pipes.
            close(QUERY_WRITE) || die $!;
            close(RESP_READ) || die $!;

	    # Duplicate our ends of the pipes to stdin and stdout.
            open(STDIN, "<&QUERY_READ") || die $!;
            open(STDOUT, ">&RESP_WRITE") || die $!;

	    # Close our origional copies if our ends of the pipes.
            close(QUERY_READ) || die $!;
            close(RESP_WRITE) || die $!;

	    # Set CGI environment variables.
            $ENV{SERVER_SOFTWARE} = $SERVER;
            $ENV{SERVER_NAME} = $request_headers->{HOST};
		$ENV{SERVER_NAME} =~ s/:\d+$//;
            $ENV{GATEWAY_INTERFACE} = "CGI/1.1";
            $ENV{SERVER_PROTOCOL} = "HTTP/$request_version_major.$request_version_minor";
            $ENV{SERVER_PORT} = ($request_headers->{HOST} =~ /:(\d+)$/) ? $1 : 80;
            $ENV{REQUEST_METHOD} = $method;
	    if($path_info ne "")
	    	{
		$ENV{PATH_INFO} = $path_info;
		$ENV{PATH_TRANSLATED} = "$WEBROOT$path_info";
		}
	    else
	    	{
	    	delete $ENV{PATH_INFO};
	    	delete $ENV{PATH_TRANSLATED};
	    	}
            $ENV{SCRIPT_NAME} = "/$http_cgi_dir/$path";
            $ENV{QUERY_STRING} = $query;
            delete $ENV{REMOTE_HOST};			# not implemented
            delete $ENV{REMOTE_IDENT};			# not implemented

	    # The method GET has no uploaded entity body.  POST and
	    # PUT do.
	    if($method eq "GET")
	    	{
		delete $ENV{CONTENT_TYPE};
		delete $ENV{CONTENT_LENGTH};
	    	}
	    else
	        {
                $ENV{CONTENT_TYPE} = $request_headers->{CONTENT_TYPE};
                $ENV{CONTENT_LENGTH} = $request_headers->{CONTENT_LENGTH};
                }

	    # Other request headers should be converted to environment variables
	    # starting with "HTTP_" as per CGI 1.1.
            my $i;
            foreach $i (keys(%$request_headers))
                {
                next if($i eq "HOST" || $i eq "CONTENT_LENGTH" || $i eq "CONTENT_TYPE" || $i eq "AUTHORIZATION");
                $ENV{"HTTP_$i"} = $request_headers->{$i};
                }

	    # Run the CGI script.
            exec("$CGI_BIN/$path") || die;
	    } ;

	# Catch exceptions
	if($@)
	    {
	    print "Status: 500\r\n";
	    print "Content-Type: text/html\r\n";
	    print "\r\n";
	    print "<html>\n";
	    print "<head>\n<title>CGI Script Error</title>\n</head>\n";
	    print "<body>\n<h1>CGI Script Error</h1>\n<p>Error: $@</p>\n</body>\n";
	    print "</html>\n";
	    exit 0;
	    }
	}

    # This is the parent.  Close the child sides of the two pipes.
    close(QUERY_READ) || die "close() failed, $!";
    close(RESP_WRITE) || die "close() failed, $!";

    # If the request method is POST then we need to send the
    # posted data to the CGI program.  There may be other
    # methods we have to do this for too.
    if($method eq "POST")
	{
	my $countdown = $request_headers->{CONTENT_LENGTH};
	if(!defined($countdown))
	    {
	    # Ouch!  No content length!
	    die "411 This $method request needs a \"Content-Length:\" header.\n";
	    }

	print STDERR "Sending $countdown bytes of form data.\n";

	my $count;
	while($countdown > 0)
	    {
	    eval
		{
		alarm($POST_REQUEST_TIMEOUT);
		$count = read(STDIN, $buffer, (($countdown > $POST_REQUEST_BUFSIZE) ? $POST_REQUEST_BUFSIZE : $countdown));
		alarm(0);
		};
	    if($@)
	    	{
	    	die unless $@ eq "alarm\n";
		die("Timeout after $POST_REQUEST_TIMEOUT seconds while waiting for POST data.");
	        }
	    if(! defined $count)
	        {
	        die("read() on socket failed, $!");
	        }
	    if($count <= 0)
	    	{
	    	die("read() returned $count while \$countdown = $countdown");
	    	}
	    print QUERY_WRITE $buffer;
	    #print STDERR $buffer;
	    $countdown -= $count;
	    }

	#print STDERR "\n";
	}

    close(QUERY_WRITE) || die "close() failed, $!";

    print STDERR "Reading response header from CGI script.\n";
    my $other_cgi_headers = "";
    my $content_length = undef;
    my $content_type = undef;
    my $status = 200;
    my $expires = undef;
    my $location = undef;
    my $last_modified = undef;
    while(<RESP_READ>)
	{
	# strip any kind of line terminator
	$_ =~ s/[\r\n]+$//;

	# Blank line ends header.
	last if($_ eq '');

	print STDERR " $_\n";

	if(/^Status:\s(\d\d\d)/)		# A CGI thing
	    {
	    $status = $1;
	    next;
	    }
	if(/^Content-Type:\s*(.+)/i)		# required if no "Location:"
	    {
	    $content_type = $1;
	    next;
	    }
	if(/^Content-Length:\s*(\d+)/i)		# a "Content-Length:" helps persistent connexions
	    {
	    $content_length = $1;
	    next;
	    }
	if(/^Expires:\s*(.+)/i)			# an "Expires:" header turns off no-cache
	    {
	    $expires = $1;
	    next;
	    }
	if(/^Location:\s*(.+)/i)		# a redirect
	    {
	    $location = $1;
	    next;
	    }
	if(/^Last-Modified:\s*(.+)/i)		# Makes "If-Modified-Since:" handling possible
	    {
	    $last_modified = cgi_time_parse($1);
	    next;
	    }

	# Save unknown headers in a string which we will later
	# dump into the header sent to the client.
	$other_cgi_headers .= $_;
	$other_cgi_headers .= "\r\n";
	}

    # Did the CGI script ask for a redirect?  If so, then use the "see other"
    # code as per RFC 2068 10.3.4.  Note that we disobey the CGI 1.1 standard
    # in that we turn non-URLs into URLs rather than feeding the client
    # the content ourselves.  This shouldn't hurt and should improve caching.
    if(defined($location))
    	{
	print STDERR "CGI script redirects to \"$location\".\n";
	# The 303 code is designed for just this purpose, but it isn't
	# defined until HTTP 1.1.  Therefor, for HTTP 1.0 we will use
	# "Temporary Redirect".
	$status = ($request_version_minor >= 1) ? 303 : 302;
	if($location =~ /^\//)
	    {
	    $location = "http://$request_headers->{HOST}/$location";
	    }
    	}

    # Sanity check
    die("CGI script didn't send a \"Content-Type:\" header.\n") unless(defined($content_type) || defined($location));

    # Determine if "If-Modified-Since:" means we should not send the entity
    # body.  This is only possible if the CGI script returned a
    # "Last-Modified:" header and the result code was 200.
    if($status == 200 && defined($last_modified) && defined(my $ims = $request_headers->{IF_MODIFIED_SINCE}))
    	{
	# Netscape 4.7 adds this spurious information:
	$ims =~ s/; length=\d+//;

	if(!defined(my $ims_parsed = cgi_time_parse($ims)))
	    {
	    print STDERR "Invalid \"If-Modified-Since: $ims\".\n";
	    }
	else
	    {
	    print STDERR "mtime: $last_modified, ims: $ims_parsed\n";
	    if($last_modified <= $ims_parsed)
	    	{
		print STDERR "Not modified.\n";
	    	$status = 304;
	    	}
	    }
    	}

    # Chunked is desireable if no content length header was received from the
    # CGI script and we will be returning an entity body.
    my $chunked = (!defined($content_length) && $status != 304) ? 1 : 0;

    # If chunked is not permitted, then we will have to close the connection
    # after this response.
    if($chunked && !($request_version_major > 1 || $request_version_minor >= 1))
    	{
    	$chunked = 0;
	$resp_header_connection = "close";
    	}

    #
    # Send the response header.  This has a series of parts:
    #
    # * The HTTP status line
    #
    # * The headers the server always sends such as "Date:"
    #
    # * Any unrecognized headers received from the CGI script
    #
    # * The "Connection:" header to tell the client whether we will disconnect
    #   after sending this CGI response.
    #
    # * If the CGI script asked for authentication, then a "WWW-Authenticate:"
    #   header with a Digest challenge.
    #
    # * Possibly a "Transfer-Encoding:" indicating that the response is
    #   chunked
    #
    # * Possibly a "Content-Length:" header (which could make chunking
    #   and closing the connection unnecessary)
    #
    # * If the CGI script did not issue an "Expires:" header, tell the client
    #   that the response it not cachable.  We do this in both HTTP 1.0 and
    #   HTTP 1.1 lingo to be sure we get the point accoss.
    #
    # * A blank line to mark the end of the header and the start of the
    #   entity body
    #
    print STDERR "Sending finished HTTP header to client.\n";
    print "HTTP/1.1 $status ", $RESPONSE_CODES{$status}, "\r\n";
    print $resp_headers_general;
    print $other_cgi_headers if($status != 304);
    print "Connection: $resp_header_connection\r\n" if($resp_header_connection ne "");

    # If the CGI script is demanding authentication, insert
    # the cryptographic challenge.
    if($status == 401)
    	{
	print STDERR "CGI script demands authentication.\n";
	my $challenge = auth_challenge($protection_domain, $stale);
	print STDERR "WWW-Authenticate: $challenge\n";
	print "WWW-Authenticate: $challenge\r\n";
    	}

    # If auth_verify() saved value for this header,
    if(defined($auth_info))
    	{
	print STDERR "Authentication-Info: $auth_info\n";
	print "Authentication-Info: $auth_info\n";
    	}

    # If the script redirected,
    print "Location: $location\r\n" if(defined($location));

    if($status != 304)
	{
	print "Transfer-Encoding: chunked\r\n" if($chunked);
	print "Content-Length: $content_length\r\n" if(defined($content_length));
	print "Content-Type: $content_type\r\n" if(defined($content_type));
	print "Last-Modified: ", cgi_time_format($last_modified), "\r\n" if(defined($last_modified));
	}

    if(defined($expires))
    	{
    	print "Expires: $expires\r\n";
    	}
    else
	{
	print "Cache-Control: no-cache\r\n";
	print "Pragma: no-cache\r\n";
	}
    print "\r\n";

    # Copy the entity body from the CGI script to the client.
    if($status != 304)
        {
        print STDERR "Copying from CGI script to client.\n";
        my $buffer;
        my $length;
        if($chunked)
            {
            print STDERR "Sending chunked response.\n";
            # Read buffers full of data
            while($length = read(RESP_READ, $buffer, $POST_RESPONSE_BUFSIZE))
                {
                # send each as a chunk
                print STDERR $length, " byte chunk\n";
                printf("%X\r\n", $length);
                print $buffer;
                print "\r\n";
                }
            # Signal end with an empty chunk.
            print "0\r\n";
            print "\r\n";
            }
        else
            {
            print STDERR "Sending unchunked response.\n";
	    my $total_length = 0;
            # Just copy buffers thru.  Either we have stated the length
            # or we will close the connection to signal the end of the
            # response.
            while($length = read(RESP_READ, $buffer, $POST_RESPONSE_BUFSIZE))
                {
                print STDERR $length, " byte block\n";
                print $buffer;
                $total_length += $length;
                }
	    if(defined($content_length) && $total_length != $content_length)
	    	{
		print STDERR "CGI script claimed wrong content length, actual length $total_length.\n";
	    	}
            }
        }

    # Close the pipe from the CGI script.
    if(!close(RESP_READ))
    	{ print STDERR "close() failed on pipe from CGI script: $!" }

    # Do we have to do this too?
    while(wait() != -1) { }

    # Return the possibly altered "Connection:" header value.
    print STDERR "CGI executing complete, status: $status, connection: $resp_header_connection\n\n";
    return $resp_header_connection;
    } # do_cgi()

#=========================================================================
# Defer to the second server.  This code is a much simplified version
# of do_cgi().
#=========================================================================
sub do_push
    {
    my($path, $request_method, $request_uri, $request_headers, $request_version_major, $request_version_minor) = @_;

    pipe(QUERY_READ, QUERY_WRITE) || die "pipe() failed, $!";

    my $pid = fork();
    defined($pid) || die "fork() failed, $!";

    # If child,
    if($pid == 0)
	{
        close(QUERY_WRITE) || die $!;
        open(STDIN, "<&QUERY_READ") || die $!;
        close(QUERY_READ) || die $!;
	exec("$HOMEDIR/lib/ppr-push-httpd") || die $!;
        }

    # If we get here, we are the parent.  Send the header to the child.
    close(QUERY_READ) || die $!;
    print QUERY_WRITE "$request_method $request_uri HTTP/$request_version_major.$request_version_minor\r\n";
    foreach my $i (keys %{$request_headers})
    	{
	my $name = $i;
	$name =~ tr/A-Z_/a-z-/;
	print QUERY_WRITE $name, ": ", $request_headers->{$i}, "\r\n";
	print STDERR " ", $name, ": ", $request_headers->{$i}, "\r\n";
    	}
    print QUERY_WRITE "\r\n";

    # If the client submitted content, send it to the child.
    my $countdown = $request_headers->{CONTENT_LENGTH};
    while($countdown > 0)
	{
	my $count;
	eval
	    {
	    alarm($POST_REQUEST_TIMEOUT);
	    $count = read(STDIN, $buffer, (($countdown > $POST_REQUEST_BUFSIZE) ? $POST_REQUEST_BUFSIZE : $countdown));
	    alarm(0);
	    };
	if($@)
	    {
	    die unless $@ eq "alarm\n";
	    die("Timeout after $POST_REQUEST_TIMEOUT seconds while waiting for POST data.");
	    }
	if(! defined $count)
	    {
	    die("read() on socket failed, $!");
	    }
	if($count <= 0)
	    {
	    die("read() returned $count while \$countdown = $countdown");
	    }
	print QUERY_WRITE $buffer;
	$countdown -= $count;
	}

    close(QUERY_WRITE) || die "close() failed, $!";
    } # do_push()

#=========================================================================
# MD5 Digest authentication as defined in RFC 2617
#
# This routine is fed the value from an "Authentication:" request header.
#
# If the credentials are valid, it will return a list containing the
# name of the authentication method, the authenticated username,
# a flag indicating whether the nonce was stale, and a string for the
# "Authentication-Info:" line.
#
# If the credentials are not valid, it returns ("", "", 0, undef).
#=========================================================================
sub auth_verify
    {
    my $request_method = shift;
    my $request_uri = shift;
    my $domain = shift;
    my $authorization = shift;
    #print STDERR "auth_verify(\$request_method=\"$request_method\", \$request_uri=\"$request_uri\", \$domain=\"$domain\", \$authorization=\"...\")\n";

    # In order to prove his identity, the user's web browsers response
    # must pass this long battery of tests.
    my @result = eval
        {
	defined($authorization) || die "Client did not attempt authentication\n";

	# Digest is the only scheme we support.
	($authorization =~ /^Digest\s(.+)$/) || die "Scheme is not Digest!\n";
	$authorization = $1;

	# Extract the name=value pairs.
        my %parm;
        while($authorization =~ m/([^ \t=]+)=(("([^"]+)")|([^ \t,]+))[ \t,]*/g)
            {
	    my $name = $1;
	    my $value = defined($4) ? $4 : $5;
            #print STDERR "name=\"$name\", value=\"$value\"\n";
            $parm{$name} = $value;
            }

	# There are all require parameters.
        defined($parm{username}) || die "username= is missing";
        defined($parm{realm}) || die "realm= is missing";
        defined($parm{qop}) || die "qop= is missing";
        defined($parm{uri}) || die "uri= is missing";
        defined($parm{nonce}) || die "nonce= is missing";
        defined($parm{nc}) || die "nc= is missing";
        defined($parm{cnonce}) || die "cnonce= is missing";
        defined($parm{response}) || die "response= is missing";

	# Make sure certain values are correct or are values we support.
        ($parm{realm} eq $REALM) || die "realm= is incorrect";
        ($parm{qop} eq "auth") || die "qop= is incorrect";
        uri_compare($parm{uri}, $request_uri) || die "uri= is incorrect";
        !defined($parm{algorithm}) || ($parm{algorithm} eq "MD5") || die "algorithm is unsupported";

        # Get HA1 from the password file.
        my $HA1 = digest_getpw($parm{username});
	#print STDERR "HA1 is \"$HA1\"\n";

        # Compute HA2 which is a hash of the request method and the request URI.
        my $HA2 = md5hex("$request_method:$parm{uri}");
	#print STDERR "HA2 is \"$HA2\"\n";

        # Compute the response that the client should have sent.
        my $correct_response = md5hex("$HA1:$parm{nonce}:$parm{nc}:$parm{cnonce}:$parm{qop}:$HA2");
	#print STDERR "Correct response is: \"$correct_response\"\n";

        # The clients computation of the digest must match ours.
        ($parm{response} eq $correct_response) || die "Response is incorrect\n";

        # Our nonces are a Unix time followed by a colon and a hash.
        ($parm{nonce} =~ /^(\d+):([0-9a-f]+)$/) || die "Nonce format not recognized\n";
        my($nonce_time, $nonce_hash) = ($1, $2);
	#print STDERR "Nonce time: $nonce_time\n";
	#print STDERR "Nonce hash: $nonce_hash\n";

        my $correct_nonce_hash = digest_nonce_hash($nonce_time, $domain);
	#print STDERR "Correct nonce hash: $correct_nonce_hash\n";
        ($nonce_hash eq $correct_nonce_hash) || die "nonce hash is incorrect.\n";

        my $time_now = time();
	#print STDERR "Time now: $time_now\n";
        if($nonce_time > $time_now || $nonce_time < ($time_now - $MAX_NONCE_AGE))
            {
            print STDERR "Nonce is too stale.\n";
            return ("", "", 1, undef);
            }

        # All tests passed, the user is ok!
	#print STDERR "Credentials pass all tests!\n";

	# Compute a proof that we too know the user's secret.  This proof
	# will go on the "Authentication-Info:" line.
        my $rspauth_HA2 = md5hex(":$parm{uri}");
        my $rspauth = md5hex("$HA1:$parm{nonce}:$parm{nc}:$parm{cnonce}:$parm{qop}:$rspauth_HA2");

	# Assemble the value for the "Authentication-Info:" line.
	my $auth_info = "qop=\"$parm{qop}\",rspauth=\"$rspauth\",cnonce=\"$parm{cnonce}\",nc=$parm{nc}";

        return ("Digest", $parm{username}, 0, $auth_info);
        };
    if($@)
        {
        my $error = $@;
        print STDERR "Digest authentication failed: $error";
	return ("", "", 0, undef);
        }
    return @result;
    }

# Split the URI into its constituent parts.  Return a
# hash reference.
sub uri_split
    {
    my $uri = shift;
    #print STDERR "uri_split(\"$uri\")\n";

    ($uri =~ m#^(([^:]+)://([^/]*))?((/[^\?]*)(\?(.*))?)?$#) || return undef;
    #          ^            ^          ^           ^
    #          scheme       host       path        query

    my %uri_hash;
    $uri_hash{scheme} = defined($2) ? $2 : "http";
    $uri_hash{scheme} =~ tr/A-Z/a-z/;
    $uri_hash{host} = defined($3) ? $3 : "";
    $uri_hash{host} =~ tr/A-Z/a-z/;
    $uri_hash{path} = defined($5) ? $5 : "/";
    $uri_hash{path} =~ s/%([0-9A-Z][0-9A-Z])/sprintf("%c",hex($1))/ige;
    $uri_hash{query} = defined($7) ? $7 : "";

    #{ my $i; foreach $i (keys(%uri_hash)) { print STDERR "$i: $uri_hash{$i}\n" } }

    return \%uri_hash;
    }

# Return true if the 2 URIs refer to the same resource.
sub uri_compare
    {
    my($uri1, $uri2) = @_;
    my($uri1_hash, $uri2_hash);

    defined($uri1_hash = uri_split($uri1)) || return 0;
    defined($uri2_hash = uri_split($uri2)) || return 0;

    ($uri1_hash->{scheme} eq $uri2_hash->{scheme}) || return 0;

    # Don't enable next line.  If you do, relative URL's
    # won't match absolute ones.
    #($uri1_hash->{host} eq $uri2_hash->{host}) || return 0;

    ($uri1_hash->{path} eq $uri2_hash->{path}) || return 0;

    # Don't enable this or it won't work with IE 5.0.
    #($uri1_hash->{query} eq $uri2_hash->{query}) || return 0;

    return 1;
    }

# Hash a string using MD5 and return the hash in hexadecimal.
sub md5hex
    {
    my $string = shift;

    #require "MD5.pm";
    #my $md5 = new MD5;
    #$md5->reset;
    #$md5->add($string);
    #return unpack("H*", $md5->digest());

    require "MD5pp.pm";
    return unpack("H*", MD5pp::Digest($string));
    }

# This function generates the hashed part of the server nonce.
sub digest_nonce_hash
    {
    my $nonce_time = shift;
    my $domain = shift;
    #print STDERR "digest_nonce_hash(\$nonce_time=\"$nonce_time\", \$domain=\"$domain\")\n";
    return md5hex("$nonce_time:$domain:$SECRET");
    }

# Find the user's entry (for the correct realm) in the private
# password file.
sub digest_getpw
    {
    my $sought_username = shift;
    #print STDERR "digest_getpw(\"$sought_username\")\n";

    my $answer = undef;

    open(PW, "<$PWFILE") || die "Can't open \"$PWFILE\", $!\n";
    while(<PW>)
	{
	chomp;
	my($username, $realm, $hash) = split(/:/);
	if($username eq $sought_username && $realm eq $REALM)
	    {
	    $answer = $hash;
	    last;
	    }
	}
    close(PW) || die;

    return $answer if(defined($answer));
    die "User \"$sought_username\" not in \"$PWFILE\"\n";
    }

#
# This routine returns the value for a "WWW-Authenticate:" header
# which makes a Digest challenge.
#
sub auth_challenge
    {
    my $domain = shift;
    my $stale = shift;
    #print STDERR "auth_challenge(\$domain=\"$domain\", \$stale=$stale)\n";

    my $nonce_time = time();
    my $nonce_hash = digest_nonce_hash($nonce_time, $domain);
    my $challenge = "Digest realm=\"$REALM\",domain=\"$domain\",qop=\"auth\",nonce=\"$nonce_time:$nonce_hash\"";

    if($stale)
    	{ $challenge .= ",stale=true" }

    return $challenge;
    }

#=========================================================================
# Digest authentication through a cookie.
#
# Yes, we do call a few routines from the section above.  We try to make
# this authentication scheme work pretty much the same way as MD5 Digest
# authentication does even though the values aren't packed in the cookie
# in the say way as they would be packed on an "Authorization:" line.
#=========================================================================
sub auth_verify_cookie
    {
    my $request_method = shift;
    my $request_uri = shift;
    my $domain = shift;
    my $cookies = shift;
    #print STDERR "auth_verify_cookie(\$request_method=\"$request_method\", \$request_uri=\"$request_uri\", \$domain=\"$domain\", \$cookies=\"...\")\n";

    my @result = eval
        {
	die "No cookies\n" if(! defined $cookies);
	my @matching_cookies = grep(/^auth_md5=/, split(/\s*[,;]\s*/, $cookies));
	die "No auth_md5 cookie\n" if(scalar @matching_cookies < 1);
	my $cookie = pop @matching_cookies;
	#print STDERR "\$cookie=\"$cookie\"\n";
	die "Cookie syntax error\n" unless($cookie =~ /^auth_md5=([^:]+):(\d+):([0-9a-f]+):([0-9a-f]+)$/);
	my($username, $nonce_time, $nonce_hash, $response) = ($1, $2, $3, $4);
	my $H1 = digest_getpw($username);
	my $correct_response = md5hex("$H1:$nonce_time:$nonce_hash");
	#print STDERR "\$correct_response=\"$correct_response\", \$response=\"$response\"\n";
	die "Password is wrong\n" if($response ne $correct_response);
        my $time_now = time();
	#print STDERR "\$time_now=$time_now, \$nonce_time=$nonce_time\n";
        die "Nonce is too stale\n" if($nonce_time > $time_now || $nonce_time < ($time_now - $MAX_NONCE_AGE));
	return ("Cookie", $username);
        my $correct_nonce_hash = digest_nonce_hash($nonce_time, $domain);
        ($nonce_hash eq $correct_nonce_hash) || die "nonce hash is incorrect.\n";
	};

    if($@)
        {
        my $error = $@;
        print STDERR "Cookie digest authentication failed: $error";
	return ("", "");
        }

    return @result;
    }

#=========================================================================
# Localhost authentication.
#
# This scheme works only on Linux.  Linux reveals the UID of each local
# socket in the /proc/net/tcp pseudo-file.  So, if the connexion is
# coming from localhost, we can look and see which user owns the socket
# on the other end of the connexion.  I suspect that this feature was
# origionaly intended for the ident server.
#=========================================================================

sub auth_verify_localhost
    {
    my($remote_port) = @_;

    my @result = eval
	{
	my $uid = undef;
	open(PORTLIST, "</proc/net/tcp") || die "Can't open /proc/net/tcp: $!\n";
	while(<PORTLIST>)
	    {
	    #print STDERR $_;
	    if(/^\s*\d+: 0100007F:([0-9A-F]{4}) 0100007F:[0-9A-F]{4}( [0-9A-F:]+){4}\s+(\d+)/)
	    	{
		#print STDERR " port: $1\n";
		my $port = hex($1);
		if($port == $remote_port)
		    {
                    $uid = $3;
		    last;
		    }
	    	}
	    }
	close(PORTLIST) || die;
	defined($uid) || die sprintf("client port 0x%4.4X not found\n", $remote_port);

	($uid >= 100) || die "UID $uid is < 100\n";

	my($user) = getpwuid($uid);

	defined($user) || die "No user for UID $uid\n";

	return ("Localhost", $user);
        } ;
    if($@)
    	{
	my $error = $@;
	print STDERR "Loopback authentication failed: $error";
	return ("", "");
    	}
    else
    	{
    	return @result;
    	}
    }

# end of file

