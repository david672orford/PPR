#! /usr/bin/wish
#
# pprpopup_loader.tcl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
# Last modified 10 January 2002.
#

package require Tcl 8.3
package require Tk 8.3

#
# Define system-dependent routines to load and save the configuration data blob.
#
switch -glob -- $tcl_platform(platform) {
    macintosh {
	proc config_load {} {
		set config [resource read TEXT 8001]
		regsub -all "\r" $config "\n" config
		return $config
		}
	proc config_save {config} {
		regsub -all "\n" $config "\r" config
		resource write -file application -id 8001 -force TEXT $config
		}
	}
    windows {
	package require registry 1.0
	proc config_load {} {
		set config [registry get "HKEY_LOCAL_MACHINE\\Software\\PPR\\PPR Popup" "config"]
		regsub -all ";" $config "\n" config
		return $config
		}
	proc config_save {config} {
		regsub -all "\n" $config ";" config
		registry set "HKEY_LOCAL_MACHINE\\Software\\PPR\\PPR Popup" "config" $config
		}
	}
    * {
	proc config_load {} {
		global env
		set f [open "$env(HOME)/.ppr/pprpopup" r]
		set config [read $f]
		close $f
		return $config
		}
	proc config_save {config} {
		global env
		catch { exec mkdir $env(HOME)/.ppr }
		set f [open "$env(HOME)/.ppr/pprpopup" w]
		puts $f $config
		close $f
		}
	}
    }

#
# This program loads the PPR popup program from a web server and runs it.
# The PPR popup program is kept on a webserver so that it can be upgraded
# easily.  While the popup program is being loaded, the PPR logo is
# displayed.
#
namespace eval pprpopup_loader {
    global ppr_root_url

    # Hide Win32 and MacOS consoles.
    catch { console hide }

    # Hide the main window.  Right now it is an empty toplevel.  We don't want
    # it to be seen until it is dressed.
    wm withdraw .

    # Pull in the HTTP routines.
    package require http 2.3

    # Define a procedure for displaying error messages.
    proc display_error {message} {
	catch { destroy .splash.scale }

	frame .splash.error
	pack .splash.error -side top -fill both -expand 1

        button .splash.error.dismiss \
        	-text "Dismiss this Error Message" \
		-background gray \
        	-command [list destroy .splash.error]
	pack .splash.error.dismiss -side bottom -fill x

        text .splash.error.text -width 20 -height 7 -wrap none
	.splash.error.text insert end $message
	scrollbar .splash.error.vsb -command ".splash.error.text yview"
	scrollbar .splash.error.hsb -orient horizontal -command ".splash.error.text xview"
	.splash.error.text configure -xscrollcommand ".splash.error.hsb set" -yscrollcommand ".splash.error.vsb set"
	pack .splash.error.vsb -side right -fill y
	pack .splash.error.hsb -side bottom -fill x
        pack .splash.error.text -side left -fill both -expand 1

        tkwait window .splash.error
        }

    # Create an image object for the PPR logo.
    set splash_image [image create photo -data {
R0lGODlhagECAfcAAAAAAAsFAAgIAAoKAA0NAA0LCxEPDxAQABUVABcXABgYABwcAB0dABER
ESANACsRACIeHjcWACAgACIiACgoACkpAC4uADAwADIyADc3ADg4ADk5AD4+ACIiIiYmJjIs
LDMtLTIyMjMzM0seAFYiAFcjAEQ7O0Q8PGsqAHYvAEBAAEREAEREDkhIAEREE0REF0REHEtH
Gk9PEVBQAFVVAFhYAFVVEVVVF1VSGVVRHlVVHVpWH0REIFVQI1VVIlVVKFVVLlVVPmZfNmZm
AGZmB2ZmDmhoAG1tAGZkEmZmFWZiHmZmHHd3AHd2B3NzDHd3CHd1Dnt7AHd3EHd0FXd3GGZm
ImZgKmZmKWZmMGZmN2ZmPnd3IHd3KHdwMXd3MHd3OEREREtLS1VKSlVLS1hNTVVVVVhYWGZa
WmZmRHduRndtTXd3QHd3SHd3UHd3WGZmZndpaX1ubnd3YHFxcXd3d4AzAKA/AIh8WIh3d4h4
eIp5eYWFAIiHCIiIAI+PAIiICpSUAJmZAJ+fAJmYCZmZC6qqAK+vALu7AL+/AIiIZIiIbYiI
dpmJdZmZcZmZe8zMAN3dAO7uAP//AIiIiIqKio6OjpaEhJmGhpmHh5mZhZmZj5mZmaqXjKqW
lq+amqqqlKqqn7ummqioqKqqqq2tra+vr7ukpLulpbu7o7u7r7u7u8y0tMzMv8zMzN3Dw93d
3e7S0u7u7v/g4P///wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACH+OCBJbWFnZSBnZW5lcmF0ZWQgYnkgQWxhZGRp
biBHaG9zdHNjcmlwdCAoZGV2aWNlPXBubXJhdykKACwAAAAAagECAQAI/gBnCRxIsKDBgwgT
KlzIsKHDhxAjSpxIsaLFixgzatzI8WKsjiBDihxJsqTJkygfqvrkBsiXJS62yGTC5InMLS64
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
}]

    # Display the PPR logo as a splash screen.
    toplevel .splash -borderwidth 2 -background black
    label .splash.label -image $splash_image
    pack .splash.label -side top
    wm overrideredirect .splash 1

    set screen_width [winfo screenwidth .splash]
    set screen_height [winfo screenheight .splash]

    # Figure out the width and height of the splash screen.  After we know it we
    # comment out the code and hard-code the values.
    #tkwait visibility .splash
    #set window_width [winfo width .splash]
    #set window_height [winfo height .splash]
    #puts "screen_width=$screen_width, screen_height=$screen_height, window_width=$window_width, window_height=$window_height"
    set window_width 370
    set window_height 266

    set window_x [expr ($screen_width - $window_width) / 2]
    set window_y [expr ($screen_height - $window_height) / 2]
    wm geometry .splash +$window_x+$window_y

    # Add a progress bar.
    scale .splash.scale \
    	-orient horizontal \
    	-from 0 -to 100 \
    	-tickinterval 10 \
    	-showvalue false
    pack .splash.scale -side top -fill x

    # Let the user see what we have so far.
    update

    # Load the configuration.
    puts "*** Loading configuration..."
    if {[catch { set config [config_load] } errormsg]} {
	puts "***    $errormsg"
	set config ""
	}

    # Execute the configuration script.
    if [catch { namespace eval :: $config }] {
	global errorInfo
	display_error "Syntax error in configuration file:\n$errorInfo"
	exit 10
	}

    # If the configuration doesn't yet have a URL to load the real pprpopup program
    # from, then put up a dialog box to prompt for it.
    if {![info exists ppr_root_url]} {

	# First we must convert the splash screen into a normal window.
	wm withdraw .splash
	wm overrideredirect .splash 0
	wm deiconify .splash

	frame .splash.pd
	label .splash.pd.label -text "Enter the name of the PPR server to load PPR Popup from\nor enter the URL of the top-level PPR directory."
	entry .splash.pd.entry -background white
	button .splash.pd.button -text "Go" -background gray -command [list destroy .splash.pd.button]
	bind .splash.pd.entry <Return> [list .splash.pd.button invoke]

	pack .splash.pd -side top -fill x
	pack .splash.pd.label -side top -fill x
	pack .splash.pd.entry -side left -fill x -expand 1 -padx 5 -pady 5
	pack .splash.pd.button -side left -padx 5 -pady 5
	focus .splash.pd.entry
	tkwait window .splash.pd.button

	.splash.pd configure -background #E0E0E0
	.splash.pd.label configure -background #E0E0E0 -foreground #505050
	.splash.pd.entry configure -state disabled -background #C0C0C0 -foreground #505050
	set ::ppr_root_url [.splash.pd.entry get]

	if {![regexp {^http://} $ppr_root_url]} {
	    set ppr_root_url "http://$ppr_root_url:15010/"
	    }
	}

    # We will call this 10% done.
    .splash.scale set 10
    update

    # Define a callback procedure to adjust the progress bar.
    # It can take the bar from 10% to 90% done.
    proc scale_callback {token total current} {
	.splash.scale set [expr $current / $current * 80.0 + 10]
        }

    # Define the callback procedure that gets called once the HTTP
    # request is finished.
    proc loaded_callback {token} {
	puts "***  Done."

	# Get rid of the progress bar.
	destroy .splash.scale
	update

	# Check to see if the GET suceeded.
	upvar #0 $token state
	#set code [::http::ncode $token]
	regexp {^HTTP/[0-9]+\.[0-9]+ ([0-9]+)} $state(http) junk ncode
	puts "*** State: $state(status)"
	puts "*** Ncode: $ncode"
	puts "*** Size: $state(totalsize)"
	if {[string compare $state(status) "ok"] != 0} {
	    display_error "Failed to load program from HTTP server:\n$state(status)\n$state(error)"
	    exit 0
	    }
	if {$ncode != 200} {
	    display_error "Failed to load program from HTTP server:\n$state(http)"
	    exit 0
	    }

	# Execute the code in the global namespace.
	puts "*** Executing downloaded code..."
	if [catch { namespace eval :: $state(body) }] {
	    global errorInfo
	    display_error "Program Execution Failed:\n\n$errorInfo"
	    exit 0
	    }

	# Remove the spash screen.
	puts "*** Removing splash screen..."
	destroy .splash
	puts "*** Freeing splash image..."
	image delete $splash_image
	puts "*** Removing loader namespace..."
	namespace delete pprpopup_loader
	puts "***   Done."
	}

    # Start the HTTP transaction.
    set code_url "${ppr_root_url}clientsoft/pprpopup.tcl"
    puts "*** Connecting..."
    if [catch {
		set token [::http::geturl $code_url \
			-progress [namespace code scale_callback] \
			-blocksize 1024 \
			-command [namespace code loaded_callback]]
	} errormsg] {
	display_error "Can't fetch <$code_url>:\n$errormsg"
	exit 10
	}
    }

# end of file
