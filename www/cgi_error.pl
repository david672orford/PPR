#
# mouse:~ppr/src/www/cgi_error.pl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 15 September 2000.
#

require 'cgi_intl.pl';

#
# This function emmits an error message as a complete HTML
# document.  The first argument is the document title, the second
# argument is the body HTML text.  Note that the title is plain
# text while the body should be HTML.
#
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

#
# This function attempts to emmit an error message as a pop-up window using
# Javascript.  If Javascript does not work, the message will appear in the
# document.  The first argument is a brief introductory message, the rest of
# the arguments are lines to present in a <pre> environment.  Both are plain
# text, not HTML.
#
# Notice that some of the strings in the write() calls are split up using the
# + operator.  This is so that the seqeuence "</" never appears in the script.
# A strict interpretation of <script></script> ends the script at the first end
# tag of any kind.
#
sub error_window
{
my $introduction = html(shift);
my @lines = @_;

#
# This is for browsers which support JavaScript.
#
{
my $intro2 = $introduction;
$intro2 =~ s/'/\\'/g;
my $height = 150 + (25 * (scalar @lines));

print <<"EndOfError1";
<script>
var w = window.open('', '_blank', 'width=600,height=$height');
var d = w.document;
d.write('<html><head><title>Operation Failed<' + '/title><' + '/head><body>\\n');
d.write('<p>$intro2<br>\\n<pre>\\n');
EndOfError1

foreach my $i (@lines)
    {
    $i = html($i);
    $i =~ s/'/\\'/g;
    print "d.write('$i\\n');\n";
    }

print <<"EndOfError2";
d.write('<' + '/pre>\\n');
d.write('<spacer type=horizontal size=300><form><input type="submit" value="Close" onclick="window.close(self)"><' + '/form>\\n');
d.write('<' + '/body><' + '/html>\\n');
d.close();
</script>
EndOfError2
}

#
# This is for browsers which don't support JavaScript.
#
{
print <<"EndOfError3";
<noscript>
<p>$introduction<br>
<pre>
EndOfError3

foreach my $i (@lines)
    {
    $i = html($i);
    print "$i\n";
    }

print <<"EndOfError4";
</pre>
</noscript>
EndOfError4
}
} # end of error_window()

1;
