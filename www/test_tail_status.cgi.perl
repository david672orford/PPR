#! /usr/bin/perl -wT
#
# mouse:~ppr/src/www/tail_status_select.cgi.perl
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 30 December 2000.
#

print <<"QUOTE50";
Content-Type: text/html

<html>
<head>
<title>PPR Raw tail_status Output</title>
</head>
<body>

<h1>PPR <tt>tail_status</tt> Viewer</h1>

<form action="/push/tail_status" method="POST">
<p>String Match 1: <input type="text" name="text1" width=20></p>
<p>String Match 2: <input type="text" name="text2" width=20></p>
<p><input type="submit" name="submit" value="Start"></p>
</form>

</body>
</html>
QUOTE50

exit 0;

