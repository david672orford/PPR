#! /bin/sh
#
# mouse:~ppr/src/create_ppr_conf.sh
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 13 March 2003.
#

#
# The purpose of this shell script is to generate ppr.conf.sample.  It will be
# installed as /etc/ppr/ppr.conf.sample.  On a clean install it will also
# be installed as /etc/ppr/ppr.conf.
#

. ../makeprogs/paths.sh

SAMPLE="ppr.conf.sample"

# Function to find a program in the specified $PATH style
# search list.  The first argument is the program to find,
# the second is the search list.
findfile_test_basename_path_default ()
    {
    echo "  Searching for $2..." >&3
    for i in `echo $3 | tr ':' ' '`
	do
	if [ `echo $i | cut -c1` = '/' ]
	    then
	    # echo "Trying $i/$2" >&3
	    if [ $1 "$i/$2" ]
		then
		echo "    Found $i/$2." >&3
		echo "$i/$2"
		return
		fi
	    fi
	done
    echo "    Not found, using $4." >&3
    echo "$4"
    }

# If the specified directory exists, print its name in
# quotes with 2 leading spaces.
if_dir_print ()
    {
    if [ -d "$1" ]
        then
	echo "  Found directory $1."
        echo "  \"$1\"" >&5
        fi
    }

# Direct file descriptor 3 to 1 so that we can print to stdout from
# within backticks
exec 3>&1

# Open the output file on file descriptor 5.
exec 5>$SAMPLE

echo "Generating $SAMPLE ..."

cat - >&5 <<===EndHere10===
#
# $SAMPLE
# Created: `date`
#

===EndHere10===

smbclient=`findfile_test_basename_path_default -x smbclient /usr/local/samba/bin:$PATH /usr/local/samba/bin/smbclient`
smb_conf=`findfile_test_basename_path_default -f smb.conf /usr/local/samba/lib:/etc:/etc/samba /usr/local/samba/lib/smb.conf`
cat - >&5 <<===EndHere30===
# Facts About Samba
[samba]

  # Where is the smbclient program which should be used for sending popup
  # messages and for sending print jobs to SMB servers?
  smbclient = "$smbclient"

  # Where is the smb.conf file that goes with that copy of smbclient?
  smb.conf = "$smb_conf"

===EndHere30===

cat - >&5 <<===EndHere40===
# What Country is this?
[internationalization]

  # What LANG environment value should the daemons be started with?
  # (The value in ppr.conf.sample (which is copied to ppr.conf if you don't
  # have one) ppr.conf was taken from the environment variable LANG when 
  # fixup was run.)
  daemon lang = $LANG

===EndHere40===

cat - >&5 <<'===EndHere41==='
  # Choose a default medium, "Letter" for the USA, A4 for
  # most other places.  You must uncomment one of these.
  default medium  = Letter, 612, 792, 75, white, ""
  #default medium = A4, 595, 842, 75, white, ""

  # Choose a money format for banner pages and ppuser output.  There should
  # be two values.  The first is a sprintf() template for positive quantities,
  # the second is a template for negative quantities.
  #
  # The first example is the default.  It doesn't specify the currency.
  # The second and third examples are for US dollars.  They differn only
  # in who negative quantities are represented.
  #money = "%d.%02d", "-%d.%02d"		# Default generic: 1.25 -1.25
  #money = "\$%d.%02d", "(\$%d.%02d)"		# USA: $1.25 ($1.25)
  #money = "\$%d.%02d", "-\$%d.%02d"		# USA: $1.25 -$1.25

  # Choose a format for dates printed on banner and trailer pages.
  # This is in strftime() format.
  #flag date format = "%d-%b-%Y, %I:%M%p"	# Default: 26-Jul-2000, 12:49pm
  #flag date format = "%d-%b-%y, %I:%M%p"	# 26-Jul-00, 12:49pm
  #flag date format = "%b %d, %Y, %I:%M%p"	# Jul 26, 2000, 12:49pm
  #flag date format = "%B %d, %Y, %I:%M%p"	# July 26, 2000, 12:49pm

===EndHere41===

cat - >&5 <<===EndHere50===
#
# Where are the Type 1 and TrueType fonts?  These directories will be
# searched recursively.
#
# After changing this section, you must run this command in order for your
# changes to take effect:
#
# $ ppr-index fonts
#
[fonts]
  "$SHAREDIR/fonts"
===EndHere50===

# X11R6
if_dir_print "/usr/X11R6/lib/X11/fonts"

# Openwindows
if_dir_print "/usr/openwin/lib/X11/fonts"

# Ghostscript
if_dir_print "/usr/local/share/ghostscript/fonts"
if_dir_print "/usr/share/ghostscript/fonts"

# RedHat Linux 6.0
if_dir_print "/usr/share/fonts"

# Adobe Acrobat Reader
if_dir_print "/usr/local/Acrobat3/Fonts"
if_dir_print "/usr/local/Acrobat4/Resource/Font"

# teTeX 1.0 on RedHat 6.0
if_dir_print "/usr/share/texmf/fonts/type1"
if_dir_print "/usr/share/texmf/fonts/afm"

echo >&5

cat - >&5 <<===EndHere90===
#
# Where are the PPD files?
#
# After changing this section, you must run this command in order for your
# changes to take effect:
#
# $ ppr-index ppds
#
[PPDs]
  "$SHAREDIR/PPDFiles"
  "/usr/share/cups/model"
  "/usr/share/cups/model/C"
  "/Library/Printers/PPDs/Contents/Resources/en.lproj

# Configuration of the new AppleTalk Printer Access Protocol server
[papd]
  #default zone = ""

# end of file
===EndHere90===

exit 0
