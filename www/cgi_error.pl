#
# mouse:~ppr/src/www/cgi_error.pl
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
# Last modified 16 April 2004.
#

require 'cgi_intl.pl';

#=============================================================================
# This function emmits an error message as a complete HTML document.  The 
# first argument is the document title, the second argument is the body HTML
# text.  Note that the title is plain text while the body should be HTML.
#=============================================================================
sub error_doc
{
my $title = html(shift);
my $text = shift;
my($charset, $content_language) = @_;

if(defined $content_language)
	{
	print "Content-Language: $content_language\n";
	}

if(defined $charset)
	{
	print "Content-Type: text/html;charset=$charset\n";
	}
else
	{
	print "Content-Type: text/html\n";
	}

print <<"EndOfErrorDoc";

<html>
<head>
<title>$title</title>
</head>
<body>
<h1>$title</h1>
$text
</body>
</html>
EndOfErrorDoc
}

#=============================================================================
# This function attempts to emmit an error message as a pop-up window using
# Javascript.  If Javascript does not work, the message will appear in the
# document.	 The first argument is a brief introductory message, the rest of
# the arguments are lines to present in a <pre> environment.  Both are plain
# text, not HTML.
#
# Notice that some of the strings in the write() calls are split up using the
# + operator.  This is so that the seqeuence "</" never appears in the script.
# A strict interpretation of <script></script> ends the script at the first
# end tag of any kind.  (Reference?)
#=============================================================================
sub error_window
{
my $introduction = shift;
my @lines = @_;

#
# This is for browsers which support JavaScript.  It tries to pop up a window,
# but if that doesn't work (due to popup blocking) is uses an alert box.
# The html() function requires JavaScript 1.2.
#
{
my $height = 150 + (25 * (scalar @lines));
sub js_preprocess
	{
	my $string = 0;
	foreach my $line (split(/\n/, shift))
		{
		if($line =~ /^\s*>>>\s*$/)
			{
			$string = 1;
			next;
			}
		if($line =~ /^\s*<<<\s*$/)
			{
			$string = 0;
			next;
			}
		if($string)
			{
			$line =~ s/^\s+//;
			$line =~ s/\s+$//;
			$line =~ s#</#<' + '/#g;
			print "+ " if($string++ > 1);
			print "'", $line, "\\n'\n";
			}
		else
			{
			print $line, "\n";
			}
		}
	}

js_preprocess <<"EndOfError1";
<script type="text/javascript">
function html(text)
	{
	text.replace('&', '&amp;');
	text.replace('<', '&lt;');
	text.replace('>', '&gt;');
	return text;
	}
var message_intro = ${\javascript_string($introduction)}; 
var message_body = ${\javascript_string(join("\\n", @lines))};
var w = window.open('', '_blank', 'width=600,height=$height,resizable');
if(!w)
	{
	alert(message_intro + '\\n\\n' + message_body);
	}
else
	{
	var d = w.document;
	d.write(
		>>>
		<html>
		<head>
		<title>Operation Failed</title>
		<style type="text/css">
		HTML {
			margin: 0px;
			}
		BODY {
			margin: 0px;
			padding: 0px;
			background-color: #EEEEEE;
			color: black;
			}
		TD {
			padding: 0.125in;
			}
		TD.text {
			padding: 0.25in;
			}
		H1 {
			font-size: 18pt;
			margin-bottom: 0.25in;
			}
		</style>
		</head>
		<body>
		<table height="$height" width="600" cellspacing="0">
		<tr>
		<td><img src="../images/exclaim.png"/></td>
		<td class="text" valign="top">
		<h1>
		<<<
		);
	d.write(html(message_intro));
	d.write(
		>>>
		</h1>
		<<<
		);
	d.write(html(message_body));
	d.write(
		>>>
		</td>
		</tr>
		<tr>
		<td>&nbsp;</td>
		<td align="right">
		<form><input type="submit" value="Close" onclick="window.close();return false;"></form>
		</td>
		</tr>
		</table>
		<script type="text/javascript">
		if(document.width)
			{
			window.resizeTo(document.width + 25, document.height + 25);
			}
		</script>
		</body>
		</html>
		<<<
		);
	d.close();
	}
</script>
EndOfError1
}

#
# This is for browsers which don't support JavaScript.
#
{
print <<"EndOfError3";
<noscript>
<p>${\html($introduction)}</p>
<pre>
EndOfError3

foreach my $i (@lines)
	{
	print html($i), "\n";
	}

print <<"EndOfError4";
</pre>
</noscript>
EndOfError4
}
} # end of error_window()

1;
