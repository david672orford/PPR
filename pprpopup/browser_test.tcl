#! /usr/bin/wish
#
# browser_test.tcl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
# Last modified 5 November 2001.
#

source ./urlfetch.itk
source ./browser.itk

#===========================================================
# Test Code
#===========================================================

if {$tcl_platform(platform) == "windows" || $tcl_platform(platform) == "macintosh"} {
    console show
    }

# Create an object for fetching URLs.
urlfetch .url

# Create a browser.
toplevel .t1
Browser .t1.sh \
	-width 6i -height 8i \
	-wrap word -linkcommand ".t1.sh import" -padx 10 \
	-hscrollmode dynamic \
	-vscrollmode dynamic \
	-getcommand [itcl::code .url get] \
	-postcommand [itcl::code .url post]
pack .t1.sh -side top -anchor w -fill both -expand 1

# Load a page into the browser.
.t1.sh import http://mouse.trincoll.edu/~chappell/pprpopup/testdoc.html
#.t1.sh import http://mouse.trincoll.edu:15010/html/test-cgi.html

# end of file
