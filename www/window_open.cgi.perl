#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/window_open.cgi.perl
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
# Last modified 7 May 2002.
#


use lib "?";
require "paths.ph";
require "cgi_data.pl";

# Read the CGI form variables.
&cgi_read_data();

my $url = cgi_data_move('url', '');
my $target = cgi_data_move('target', '_blank');
my $resizable = cgi_data_move('resizable', '');
my $scrollbars = cgi_data_move('scrollbars', '1');
my $width = cgi_data_move('width', 600);
my $height = cgi_data_move('height', 400);

my $options = "width=$width,height=$height";
$options .= ",resizable" if($resizeable);
$options .= ",scrollbars" if($scrollbars);

print <<"Head";
Content-Type: text/html

<html>
<head>
</head>
<body>
<script>
window.open('$url', '$target', '$options');
window.close();
</script>
</body>
</html>
Head

exit 0;
