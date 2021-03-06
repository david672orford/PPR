#! /bin/sh
#
# mouse:~ppr/src/create_ppr_conf.sh
# Copyright 1995--2012, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified: 31 October 2012
#

#
# The purpose of this shell script is to generate ppr.conf.sample.	It will be
# installed as /etc/ppr/ppr.conf.sample.  On a clean install it will also
# be installed as /etc/ppr/ppr.conf.
#

. ../makeprogs/paths.sh

SAMPLE="ppr.conf.sample"

# Function to find a program in the specified $PATH style
# search list.	The first argument is the program to find,
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
rm -f $SAMPLE
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
  # most other places.	You must uncomment one of these.
  default medium  = Letter, 612, 792, 75, white, ""
  #default medium = A4, 595, 842, 75, white, ""

  # Choose a money format for banner pages and ppuser output.  There should
  # be two values.	The first is a sprintf() template for positive quantities,
  # the second is a template for negative quantities.
  #
  # The first example is the default.  It doesn't specify the currency.
  # The second and third examples are for US dollars.  They differn only
  # in who negative quantities are represented.
  #money = "%d.%02d", "-%d.%02d"				# Default generic: 1.25 -1.25
  #money = "\$%d.%02d", "(\$%d.%02d)"			# USA: $1.25 ($1.25)
  #money = "\$%d.%02d", "-\$%d.%02d"			# USA: $1.25 -$1.25

  # Choose a format for dates printed on banner and trailer pages.
  # This is in strftime() format.
  #flag date format = "%d-%b-%Y, %I:%M%p"		# Default: 26-Jul-2000, 12:49pm
  #flag date format = "%d-%b-%y, %I:%M%p"		# 26-Jul-00, 12:49pm
  #flag date format = "%b %d, %Y, %I:%M%p"		# Jul 26, 2000, 12:49pm
  #flag date format = "%B %d, %Y, %I:%M%p"		# July 26, 2000, 12:49pm

  # Choose a format job submission dates or times as displayed by the ppop
  # command.  The first value is the format for jobs at least 24 hours
  # old, the second for younger jobs.  If nothing is defined here, or the 
  # format you define is invalid or contains elements which aren't defined in
  # the current locale, then the first defaults to "%d-%b-%y" (day-of-month,
  # localized-abbreviated-month-name, two digit year) while the second defaults
  # to "%I:%M%p" (12-hour hour, two digit minutes, local-specific AM/PM 
  # indicator) for locales which define AM/PM indicators and "%H:%M" (24-hour
  # hour, two digit minutes) for those which do not.
  #
  # The default is not "%x" and "%X" because, at least in Glibc 2.3.2 1) %x and
  # %X don't seem to be defined for many locales, and 2) the C locale produces 
  # 24 hour time which is all but incomprehensible to most Americans.  To fix
  # this, uncomment the line below.
  #ppop date format = "%x", "%X"			# prefered local convention

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
# List all directories which contain PPD files which you wish to make readily
# available for use by PPR.  The listed directories will be searched
# recursively.  You needn't list PPR's own PPD file directory since it will be
# automatically included.
#
# After changing this section, you must run this command in order for your
# changes to take effect:
#
# $ ppr-index ppds
#
# You should also re-run this commands whenever files are added to or removed
# from any of the listed directories.
#
[PPDs]
===EndHere90===

# MacOS 10.x
if_dir_print "/Library/Printers/PPDs/Contents/Resources/en.lproj"

# PPR's Ghostscript distribution
if_dir_print "/usr/share/ppr-gs/ppd"

# CUPS
if_dir_print "/usr/share/cups/model"
if_dir_print "/usr/share/cups/ppd"

# Debian
if_dir_print "/usr/share/postscript/ppd"
if_dir_print "/usr/share/ppd"

echo >&5

cat - >&5 <<===EndHere100===
# Configuration of the new AppleTalk Printer Access Protocol server
[papd]
  #default zone = ""

# end of file
===EndHere100===

exit 0
