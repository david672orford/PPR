#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/window_open.cgi.perl
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

=pod

The script ppr-web may instruct the browser to load this script from
localhost.	This script in turn will load the page that the users really
wants to see.  Why, you ask, not just load the desired page in the first
place?	Because we want to load that page without those user interface
elements which are part of the web browser.	 This is easier said than done
since browsers often forbid this because it is not desirable that web sites
should be able to deprive the user of the controls to his web browser.	Of
course, ppr-web-control is launched as an application, so being nice to the
user isn't an issue, but the protects still get in our way.

There are two possible solutions.  The first is to have the first
(un-undecoratable) browser window open a second one without decorations (one
is allowed to do that) and then close the original window (one is allowed to
do that too since it is initial page and gets a special dispensation).
Another wrinkle is that Mozilla can partially disable window.open() so that
it only works if it is called as the result of a user action such a clicking
on a button.  That is why this script puts a button on the intermediate page
which the user can press if the automatic call to window.open() fails.

We have implemented the above described ugly popup, popup a child and kill
thyself hack.

The second method involves the Netscape security manager and requsting
sufficient privledges to remove the troublesome decorations.  Normally
Mozilla/Netscape will deny such a request without even consulting the user,
but there are two exceptions.  The first is if the script is signed, the
second is if it was loaded from a file:// URL.	We have implemented this
method as well, but in ppr-web-control itself.	It writes the 'nasty code'
to a file in the user's PPR preferences directory and has Mozilla load that.

So, Mozilla uses the trusted script method while other browers such as
Konqueror still bounce off of this script here.

=cut

use lib "?";
require "paths.ph";
require "cgi_data.pl";

# Read the CGI form variables.
&cgi_read_data();

# The URL and all of the window options for the window the user really wants
# to see are in the query string.
my $url = cgi_data_move('url', '');
my $target = cgi_data_move('target', '_blank');
my $resizable = cgi_data_move('resizable', '');
my $scrollbars = cgi_data_move('scrollbars', '1');
my $width = cgi_data_move('width', 600);
my $height = cgi_data_move('height', 400);

# Assemble some of the options into the second argument for window.open(). 
my $options = "width=$width,height=$height";
$options .= ",resizable" if($resizeable);
$options .= ",scrollbars" if($scrollbars);

print <<"Head";
Content-Type: text/html

<html>
<head>
<title>Transient PPR Window</title>
</head>
<body>

<p>This window should go away in a moment.	If it doesn't, you have 
probably outsmarted yourself in an attempt to avoid popup ads.
</p>
<form>
<input name="manualy" type="button" value="Trying..." onclick="open_ppr()">
</form>

<script>
window.resizeTo(500, 300);
function open_ppr ()
	{
	if(window.open('$url', '$target', '$options') != null)
		window.close();
	else
		document.forms[0].manualy.value = "Yes, you have, click here please.";
	}
open_ppr();
</script>

</body>
</html>
Head

exit 0;
