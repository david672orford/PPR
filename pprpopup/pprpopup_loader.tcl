#! /usr/local/bin/wish8.4
#! /usr/bin/wish
#
# pprpopup_loader.tcl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
# Last modified 28 March 2002.
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
    variable splash_image
    set splash_image [image create photo -data {
R0lGODlhkAEdAcIAAAAAAKA/AP/g4I6Ojv//////AO/v7wAAACwAAAAAkAEd
AQAD/mi63P4wyumAvTTrzbv/YCiOZGmeaLpd7FW8cNuqdG3feK7vfMnCwKBw
WGD1jsikcslsan7EqFRodFqv2KwW65p6v0HMdkwum8+RLnjNLlrQ8Lh8blO3
7+w3fc/v+xkAeIKCen+Gh4hOFoOMeIWJkJGSJ4uNlnePkTKbMxWcnxCfnJOk
I5WXqG2ZkKKjnq2dDbCbpbVPgam5mAClsQquslULwIDCwzK2yQ+nus15pL7F
xr/Tx9XR0qvKvc7du5LY1qvh1GLZvGnV2+De7aqa6uXm8ujL0eT09evs7v1g
+obwGbgXj16wefYQ7jvEzJ/DKAD9CCSo8NxBbRYXJsL1/rCjFIxzJk4TiVDg
wIIaQ3pcOSUiHZKZYAI0aTIlGo4sc1L5I/OiS3EzUSKz2QenzqMxJBYcCjTd
SKEoiZZpiLQqSDP4sGW9tjSq1DFGq4otwodcOLPqztb8uiXsWKt7tDLNeE7b
LK9stbh9Czfk3Z8n7zptlTcu38NJ5QiWsHhwvbWFs+xF3BcO5FdXGRu7HLnJ
ZMqVz3Cm28HX3M5gQasGrBdvwsyOfbJG7Vn1asuuMc+O7ZN2atu3Refu/UFt
Rd9LPgMXu9vzcNIc0B5HfkT5cuZYnzctHu809erXlzdPrt0g9+nev++wHj50
6+kTRm8PJV+9qfbip5YPDJs+/nz+49lnAnv4IRVgdfvVB2B8CgpYWoHXHbge
LdFRuEJ6dUno4IMQAqdhHbMwCAsFI/rX4IZpdBghiizW1o0MzLXQzoct1njf
i5zo9MmMNvaoA4G75JiTKN7Q6OORJOJI5EqwFInkk5QouSOTrTgJ5ZUhABnk
kg/NwiOWYEZXZJMdeWllmGgyNiOZZVZ5ZppwyrIml23S6YyRcbKopSN2OuTm
l3nGuSchQrI0pTt4BirgoITKeNQmXSoKJ6ODUGUgpZVKmiamKmKnKZicduop
En/99+kVoYpqIBOl9ndqcqp2mqgPT2H4KquxijqrKd2deKsKqeaqo3Po7fcr
DcEK/msosSDZemwPySrr0a4i9PSsi9LiR22WUJl67Q3RZhspq926+i2y4kK4
7XlXbQWZPr6SCF1k4abrz7oeWHvQfJjlU5JpFmbjr7nr1GsvouQWW+truvFC
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
g98ngzPYWTXIezeIg9bXfDtIgi04az8IhMung0NIWT3oekeIhMmnggc4hVTY
TAm4gFi4cw3oeVCIflJYhWAYhqJwhVlYhja3hRbQhV4ohEsYZUUoa0+ohurH
hm04Wk24eXEoh7SnhHXYZW84dgCih0FYdn0YXn+YankoiPNHh4XIhIeodh+o
iODHiI1oiI/oaYkoiaXHh5Vo/ld36HeZqImjx4mdqF6XiImRKIq/94Vi2Iqu
SIZmmIVoGHuquIqUGGKumIthCIuxuICzWIu2SIilOGaneHepCIyzR4rDWGfF
WGmhiIzKuIxg9Yl294zAGI3SqF/N+HcfhIy1h43ZWFXU6HbT542yB47hyFTj
mHa+Z46LKIzpGFbrKHaW547MB4/x+FXzyHWdJ4aBpYsAeYC82Iv893qw54+V
FZAKeXkDSZCTZ5AHCYb5yFn7eHWAJ5ET6YjbuGh4V4UZqZEbmWhr95F+GJIi
SXZNR5JpZpInGWQpqZLyyJK9h5JUB5OmKJOGNpI26Yk4mZM0+Y8LGZR515AO
GXkQ/plpNYlQQrmUcUeURXl4R5llL7mTVlWRUaeTVClfPSl9P5mVPLmV3NeV
XnmTYGl+YjmWK1mWZumSSYmW4qiWa3lkU+mWQWWVWMeWbUmXS2WXXYeXesmM
iPaUstiScjmXfzlhTimYSBeVX4aPh6lhcDl1TDmZVRiZkkmZmIl/lnmZmdmZ
Q6mWIueZorlnmxlzo3mau1WatlRJfPlxq0lKrWlxr/lJhaeYBUmYs0lItWmb
D4mbublHselwvwlJwfl2w6mbqnmcbVSc/KacgMScnOecdgSd6yad05mc1nkt
1Clu2bmc2NmdxPSd4Pkq23l84/kt5fls54me4rmekpKeJM7mns8Cn8Ymn8dC
n+9nn7eCnx6on6fCny7on/+pkAJaoDSQAAA7
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

    # Define a callback procedure to adjust the progress bar.
    # It can take the bar from 20% to 90% done.
    proc scale_callback {token total current} {
	.splash.scale set [expr $current / $current * 70.0 + 20]
        }

    # Define the callback procedure that gets called once the HTTP
    # request is finished.
    proc loaded_callback {token} {
	puts "***  Done."

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

	# We really are 100% done.
	.splash.scale set 100

	# Remove the splash screen in a moment.
	after 500 [namespace code { splash_remove }]
	}

    # Remove the spash screen.
    proc splash_remove {} {
	variable splash_image

	puts "*** Removing splash screen..."
	destroy .splash

	puts "*** Freeing splash image..."
	image delete $splash_image

	puts "*** Removing loader namespace..."
	namespace delete ::pprpopup_loader

	puts "***   Done."
	}

    # We will call this 10% done.
    .splash.scale set 10
    update

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

    # We will call this 20% done.
    .splash.scale set 20
    }

# end of file