#! @PPR_TCLSH@
#
# mouse:~ppr/src/www/ppr-web.tcl
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 13 January 2005.
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

# Process the command line arguments.  The first one is a queue name and any
# that follow are file names.
set opt_d 0
set queue ""
set files {}
foreach arg $argv {
	if {[string compare $arg "-d"] == 0} {
		set opt_d 1
		} else {
		if {$opt_d == 1} {
			set queue $arg
			set opt_d 0
			} else {
			if {[string compare $queue ""] == 0} {
				puts stderr "Warning: specifying a queue name without -d is deprecated"
				set queue $arg
				} else {
				lappend files $arg
				}
			}
		}
    }

# If some files were to be printed, print them now and exit.  This is to
# support drag-and-drop when we are a desktop icon.
if {$files != {}} {
    foreach file $files {
		puts "Printing file $file..."
		set result [catch {exec ppr -d $queue -e responder $file >@stdout 2>@stderr} error]
		if {$result != 0} {
		    puts "Failed to print file \"$file\" due to error: $error"
		    exit 1
		    }
    	}
    exit 0
    }

# If a printer was specified, we show its queue, otherwise we show the PPR
# control panel.
if {$queue != ""} {
    set final_url "$base_url/cgi-bin/show_jobs.cgi?name=$queue"
    set width 800
    set height 300
    } else {
    set final_url "$base_url/cgi-bin/show_queues.cgi"
    set width 700
    set height 400
    }

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
    if {$selected_browser != ""} {
		break
		}
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
	
		if {![file isdirectory $env(HOME)/.ppr ]} {
			puts "Creating directory \"$env(HOME)/.ppr\"..."
			exec mkdir $env(HOME)/.ppr
			}
	
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
				if(!window.open('$final_url', '_self', '', true))
					{
					/* alert('Popup blocking has messed up the close button.'); */
					window.location = '$final_url';
					}
				}
			</script>
			</head>
			<body onload=\"doit()\">
			<p>Javascript code executing in this window is trying to 
			remove some of its user interface features which, while appropriate
			for browsing the WWW, only get in the way of the PPR web interface.
			You browser may ask for your permission to allow these operations.  
			If it does, please check the box which says \"remember this descision\"
			and then press the <b>Yes</b> button.</p>
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
