#! ppr-tclsh
#
# mouse:~ppr/src/www/ppr-web-control.tcl
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

#
# This script attempts to open the PPR web interface in the best web
# browser available (for our purposes that is).
#

# This is the url of the PPR root.
set base_url "http://localhost:15010"

# This is a script which uses Javascript to open the window without
# decoration and then closes itself.
set opener_url "$base_url/cgi-bin/window_open.cgi"

# This is the URL of the page we really want to open.
set final_url "$base_url/cgi-bin/show_queues.cgi"

# This is the size of the second window.
set width 700
set height 550

# These are the browsers in order of preference.  Again, this is according
# to how well or poorly they render the PPR web interface.  They are listed
# separately for X11 and character mode.
set browsers_x11 [list mozilla konqueror]
set browsers_character [list links w3m lynx]

set browsers ""
if [info exists env(DISPLAY)] {
    set browsers $browsers_x11
    }
set browsers [concat $browsers $browsers_character]

# We look for each in turn in the path.
set selected_browser ""

puts "Searching for suitable web browser..."
foreach browser $browsers {
    puts "  $browser..."
    foreach dir [split $env(PATH) :] {
	#puts "    in $dir..."
	if [file executable $dir/$browser] {
	    set location $dir
	    set selected_browser $browser
	    break
	    }
	}
    if {$selected_browser != ""} { break }
    }

if {$selected_browser == ""} {
    puts "No web browser found.";
    exit 1
    }

# Now we launch the selected browser.
switch -exact $selected_browser {
    mozilla {

	# Obsolete bounce-off-cgi-script method
	#exec $location/mozilla -remote \
	#	"openurl($opener_url?url=$final_url;width=$width;height=$height,new-window)" \
	#	>@stdout 2>@stderr

	set file [open $env(HOME)/.ppr/ppr-web-control.html w 0600]

	puts $file "<html>
		<head>
		<title>Transient PPR Window</title>
		<script>
		function doit ()
			{
			try	{
				netscape.security.PrivilegeManager.enablePrivilege('UniversalBrowserWrite');
				window.locationbar.visible = false;
				window.toolbar.visible = false;
				window.statusbar.visible = false;
				window.menubar.visible = false;
				window.personalbar.visible = false;
				}
			catch(e) { }
			window.resizeTo($width,$height);
			window.location = '$final_url';
			}
		</script>
		</head>
		<body onload=\"doit()\">
		<p>This window is trying to remove user interface features
		which are appropriate for browsing the WWW but only get in
		the way of the PPR web interface.  You browser will ask for
		your permission to allow these operations.  Please check
		the box which says \"remember this descision\" and then press
		the Yes button.</p>
		</body>
		</html>"
	close $file

	# first we try to contact an already-running copy
	puts "Contacting Mozilla..."
	set result [catch {
		exec $location/mozilla -remote \
			"openurl(file://$env(HOME)/.ppr/ppr-web-control.html,new-window)" \
			>@stdout 2>@stderr
		} error]

	if {$result != 0} {
	    #puts "\$result=$result, \$error=$error"
	    puts "Mozilla isn't running yet, starting it..."
	    exec $location/mozilla \
		"$env(HOME)/.ppr/ppr-web-control.html" \
		>@stdout 2>@stderr &
	    }

	}
    konqueror {
	exec $location/konqueror "$opener_url?url=$final_url;width=$width;height=$height" \
		>@stdout 2>@stderr
	}
    links {
	exec $location/links $final_url \
		>@stdout 2>@stderr <@stdin
	}
    w3m {
	exec $location/w3m $final_url \
		>@stdout 2>@stderr <@stdin
	}
    lynx {
	exec $location/lynx $final_url \
		>@stdout 2>@stderr <@stdin
	}
    }

puts "Done."

exit 0
