#! @SHELL@
#
# mouse:~ppr/src/filters_misc/indexfilters.sh
# Copyright 1995--2012, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified: 4 December 2012
#

#
# This program is used to search the current path for programs which
# are suitable for use as input filters and create filter scripts
# which use them.
#

# System configuration information:
LIBDIR="@LIBDIR@"
CONFDIR="@CONFDIR@"
PRINTERS_PURGABLE_STATEDIR="@PRINTERS_PURGABLE_STATEDIR@"
TEMPDIR="@TEMPDIR@"
USER_PPR=@USER_PPR@
GROUP_PPR=@GROUP_PPR@
FILTDIR="@FILTDIR@"

# Derived things
STORE="$LIBDIR/fixup"
TARGET="$RPM_BUILD_ROOT$FILTDIR"
PPR_TCLSH="$LIBDIR/bin/ppr-tclsh";

# Function to find a program in the $PATH list.
findprog_prog ()
	{
	echo "Searching for $1..." >&3
	for i in `echo $PATH | tr ':' ' '`
		do
		if [ `echo $i | cut -c1` = '/' ]
			then
			# echo "Trying $i/$1..." >&3
			if [ -x "$i/$1" ]
				then
				echo "  Found $i/$1." >&3
				echo "$i/$1"
				return
				fi
			fi
		done
	echo "  Not found." >&3
	}

# Run a shell script thru Sed, changing the program paths
sedit ()
	{
	echo "  $1 --> $2"
	sed \
		-e "s#\@PPR_TCLSH\@#$PPR_TCLSH#g" \
		-e "s#\@PR\@#$PR#g" \
		-e "s#\@LIBDIR\@#$LIBDIR#g" \
		-e "s#\@TEMPDIR\@#$TEMPDIR#g" \
		-e "s#\@FILTDIR\@#$FILTDIR#g" \
		-e "s#^\\(\$*\\)TROFF=\"[^\"]*\"#\\1TROFF=\"$TROFF\"#" \
		-e "s#^\\(\$*\\)EQN=\"[^\"]*\"#\\1EQN=\"$EQN\"#" \
		-e "s#^\\(\$*\\)REFER=\"[^\"]*\"#\\1REFER=\"$REFER\"#" \
		-e "s#^\\(\$*\\)TBL=\"[^\"]*\"#\\1TBL=\"$TBL\"#" \
		-e "s#^\\(\$*\\)PIC=\"[^\"]*\"#\\1PIC=\"$PIC\"#" \
		-e "s#^\\(\$*\\)DPOST=\"[^\"]*\"#\\1DPOST=\"$DPOST\"#" \
		-e "s#^\\(\$*\\)POSTREVERSE=\"[^\"]*\"#\\1POSTREVERSE=\"$POSTREVERSE\"#" \
		-e "s#^\\(\$*\\)GROFF=\"[^\"]*\"#\\1GROFF=\"$GROFF\"#" \
		-e "s#^\\(\$*\\)GROPS=\"[^\"]*\"#\\1GROPS=\"$GROPS\"#" \
		-e "s#^\\(\$*\\)GROG=\"[^\"]*\"#\\1GROG=\"$GROG\"#" \
		-e "s#^\\(\$*\\)TEX=\"[^\"]*\"#\\1TEX=\"$TEX\"#" \
		-e "s#^\\(\$*\\)LATEX=\"[^\"]*\"#\\1LATEX=\"$LATEX\"#" \
		-e "s#^\\(\$*\\)TEXI2DVI=\"[^\"]*\"#\\1TEXI2DVI=\"$TEXI2DVI\"#" \
		-e "s#^\\(\$*\\)DVIPS=\"[^\"]*\"#\\1DVIPS=\"$DVIPS\"#" \
		-e "s#^\\(\$*\\)ACROREAD=\"[^\"]*\"#\\1ACROREAD=\"$ACROREAD\"#" \
		-e "s#^\\(\$*\\)PDFTOPS=\"[^\"]*\"#\\1PDFTOPS=\"$PDFTOPS\"#" \
		-e "s#^\\(\$*\\)DJPEG=\"[^\"]*\"#\\1DJPEG=\"$DJPEG\"#" \
		-e "s#^\\(\$*\\)GIFTOPNM=\"[^\"]*\"#\\1GIFTOPNM=\"$GIFTOPNM\"#" \
		-e "s#^\\(\$*\\)PPMTOPGM=\"[^\"]*\"#\\1PPMTOPGM=\"$PPMTOPGM\"#" \
		-e "s#^\\(\$*\\)PNMTOPS=\"[^\"]*\"#\\1PNMTOPS=\"$PNMTOPS\"#" \
		-e "s#^\\(\$*\\)BMPTOPPM=\"[^\"]*\"#\\1BMPTOPPM=\"$BMPTOPPM\"#" \
		-e "s#^\\(\$*\\)XBMTOPBM=\"[^\"]*\"#\\1XBMTOPBM=\"$XBMTOPBM\"#" \
		-e "s#^\\(\$*\\)XPMTOPPM=\"[^\"]*\"#\\1XPMTOPPM=\"$XPMTOPPM\"#" \
		-e "s#^\\(\$*\\)XWDTOPNM=\"[^\"]*\"#\\1XWDTOPNM=\"$XWDTOPNM\"#" \
		-e "s#^\\(\$*\\)PNMDEPTH=\"[^\"]*\"#\\1PNMDEPTH=\"$PNMDEPTH\"#" \
		-e "s#^\\(\$*\\)TIFFTOPNM=\"[^\"]*\"#\\1TIFFTOPNM=\"$TIFFTOPNM\"#" \
		-e "s#^\\(\$*\\)PNGTOPNM=\"[^\"]*\"#\\1PNGTOPNM=\"$PNGTOPNM\"#" \
		-e "s#^\\(\$*\\)PLOT2PS=\"[^\"]*\"#\\1PLOT2PS=\"$PLOT2PS\"#" \
		-e "s#^\\(\$*\\)POSTPLOT=\"[^\"]*\"#\\1POSTPLOT=\"$POSTPLOT\"#" \
		-e "s#^\\(\$*\\)FIG2DEV=\"[^\"]*\"#\\1FIG2DEV=\"$FIG2DEV\"#" \
		-e "s#^\\(\$*\\)HTMLDOC=\"[^\"]*\"#\\1HTMLDOC=\"$HTMLDOC\"#" \
		-e "s#^\\(\$*\\)LIBDIR=\"[^\"]*\"#\\1LIBDIR=\"$LIBDIR\"#" \
		-e "s#^\\(\$*\\)FILTDIR=\"[^\"]*\"#\\1FILTDIR=\"$FILTDIR\"#" \
		-e "s#^\\(\$*\\)CONFDIR=\"[^\"]*\"#\\1CONFDIR=\"$CONFDIR\"#" \
		-e "s#^\\(\$*\\)PRINTERS_PURGABLE_STATEDIR=\"[^\"]*\"#\\1PRINTERS_PURGABLE_STATEDIR=\"$PRINTERS_PURGABLE_STATEDIR\"#" \
		-e "s#^\\(\$*\\)TEMPDIR=\"[^\"]*\"#\\1TEMPDIR=\"$TEMPDIR\"#" \
		$1 >$2 || exit 1 # infile, outfile

	# Make it executable
	chmod 755 $2 || exit 1

	# This is so that if we are root, ppr will own the file.
	chown $USER_PPR $2 || exit 1
	chgrp $GROUP_PPR $2 || exit 1
	}

# Make sure we have permission.
if [ ! -w $TARGET ]
	then
	echo "In order to run this program, you must have write access"
	echo "to the directory \"$TARGET\".  You do not."
	exit 1
	fi

if [ "$1" != "" ]
	then
	if [ "$1" = "--delete" ]
		then
		for t in pr ditroff troff dvi tex texinfo pdf html jpeg gif bmp pnm xbm xpm xwd tiff png plot fig
			do
			rm -f $TARGET/filter_$t
			done
		exit 0
		else
		echo "lib/indexfilters: unknown option: $1"
		exit 1
		fi
	fi

echo "===================================================="
echo "= Setting up filters                               ="
echo "===================================================="

# Direct file descriptor 3 to 1 so that we can print to stdout from
# within backticks
exec 3>&1

echo "=== AT&T Troff ==="

# look for dpost
echo "Searching for dpost..."
if [ -x /usr/lib/lp/postscript/dpost ]
	then
	DPOST="/usr/lib/lp/postscript/dpost"
	echo "  Found $DPOST."
	else
	DPOST=""
	echo "  Not found."
	fi

# look for postreverse
echo "Searching for postreverse..."
if [ -x /usr/lib/lp/postscript/postreverse ]
	then
	POSTREVERSE="/usr/lib/lp/postscript/postreverse"
	echo "  Found $POSTREVERSE."
	else
	POSTREVERSE=""
	echo "  Not found."
	fi

TROFF=`findprog_prog troff`

echo "=== GNU Troff ==="

GROFF=`findprog_prog groff`
GROPS=`findprog_prog grops`
GROG=`findprog_prog grog`

echo "=== TeX ==="
TEX=`findprog_prog tex`
LATEX=`findprog_prog latex`
TEXI2DVI=`findprog_prog texi2dvi`
DVIPS=`findprog_prog dvips`

echo "=== Paginator ==="
PR=`findprog_prog pr`

echo "=== HTML ==="
HTMLDOC=`findprog_prog htmldoc`

echo "=== PDF ==="
ACROREAD=`findprog_prog acroread`
PDFTOPS=`findprog_prog pdftops`

echo "=== Raster Image Converters ==="
DJPEG=`findprog_prog djpeg`
GIFTOPNM=`findprog_prog giftopnm`
if [ -z "$GIFTOPNM" ]; then GITTOPNM=`findprog_prog giftoppm`; fi
PPMTOPGM=`findprog_prog ppmtopgm`
PNMTOPS=`findprog_prog pnmtops`
BMPTOPPM=`findprog_prog bmptoppm`
XBMTOPBM=`findprog_prog xbmtopbm`
XPMTOPPM=`findprog_prog xpmtoppm`
XWDTOPNM=`findprog_prog xwdtopnm`
PNMDEPTH=`findprog_prog pnmdepth`
TIFFTOPNM=`findprog_prog tifftopnm`
PNGTOPNM=`findprog_prog pngtopnm`

echo "=== Plot ==="
echo "Searching for postplot..."
if [ -x /usr/lib/lp/postscript/postplot ]
	then
	POSTPLOT="/usr/lib/lp/postscript/postplot"
	echo "  Found $POSTPLOT."
	else
	POSTPLOT=""
	echo "  Not found."
	fi
PLOT2PS=`findprog_prog plot2ps`

echo "=== Vector Formats ==="
FIG2DEV=`findprog_prog fig2dev`

# Separate next section
echo

# If we have both Troff and Groff, eliminate Troff
# if it is just a link to GNU troff.
if [ -n "$TROFF" -a -n "$GROFF" ]
	then
	if $TROFF -v /dev/null 2>&1 | grep GNU >/dev/null
		then
		echo "Troff is a link to Groff, ignoring it."
		TROFF=""
		fi
	fi

# Choose a Groff over Troff.
if [ -n "$TROFF" -a -n "$GROFF" ]
	then
	echo "Choosing Groff rather than Troff."
	TROFF=""
	fi

# Choose a Ditroff output to PostScript converter
if [ -n "$DPOST" -a -n "$POSTREVERSE" -a -n "$GROPS" ]
	then
	echo "Choosing Grops rather than dpost and postreverse."
	DPOST=""
	POSTREVERSE=""
	fi

# Choose a plot filter
if [ -n "$POSTPLOT" -a -n "$PLOT2PS" ]
	then
	echo "Choosing plot2ps in preference to postplot."
	POSTPLOT=""
	fi

# Choose an Acrobat filter.
if [ -n "$ACROREAD" -a -n "$PDFTOPS" ]
	then
	#echo "Choosing Acroread over pdftops (Xpdf)."
	#PDFTOPS=""
	echo "Choosing pdftops (Xpdf) over Acroread."
	ACROREAD=""
	fi

# Separate next section
echo

#
# Install the PR filter
#
rm -f $TARGET/filter_pr
HAVE_PR=""
if [ -n "$PR" ]
	then
	echo "Installing PR filter."
	sedit "$STORE/filter_pr" "$TARGET/filter_pr"
	HAVE_PR="YES"
	fi
if [ -z "$HAVE_PR" ]
	then
	echo "No PR filter."
	fi

#
# Install Ditroff output filter
#
rm -f $TARGET/filter_ditroff			# remove old filter
HAVE_DITROFF=""							# say we don't have one yet
if [ -n "$GROPS" ]						# try Grops
	then
	echo "Installing filter for Ditroff output (from the Groff package)."
	sedit $STORE/filter_ditroff_groff $TARGET/filter_ditroff
	HAVE_DITROFF="YES"
	fi
if [ -n "$DPOST" -a -n "$POSTREVERSE" ] # try dpost/postreverse
	then
	echo "Installing filter for Ditroff output (dpost, postreverse)."
	sedit $STORE/filter_ditroff_real $TARGET/filter_ditroff
	HAVE_DITROFF="YES"
	fi
if [ -z "$HAVE_DITROFF" ]
	then
	echo "No Ditroff output filter."
	fi

#
# Install Troff filter
# This is only installed if a Ditroff output filter was installed.
#
rm -f $TARGET/filter_troff		# remove old filter
HAVE_TROFF=""							# say we don't have one yet
if [ -n "$GROFF" -a -n "$HAVE_DITROFF" ]
	then
	echo "Installing Troff filter (From the Groff package)."
	sedit $STORE/filter_troff_groff $TARGET/filter_troff
	HAVE_TROFF="YES"
	fi
if [ -n "$TROFF" -a -n "$HAVE_DITROFF" ]
	then
	echo "Installing Troff filter (AT&T Troff)."
	# find the Troff components
	EQN=`findprog_prog eqn`
	REFER=`findprog_prog refer`
	TBL=`findprog_prog tbl`
	PIC=`findprog_prog pic`
	CAT=`findprog_prog cat`
	# substitute cat for those we don't have
	if [ -z "$EQN" ]; then EQN="$CAT"; fi
	if [ -z "$TBL" ]; then TBL="$CAT"; fi
	if [ -z "$REFER" ]; then REFER="$CAT"; fi
	if [ -z "$PIC" ]; then PIC="$CAT"; fi
	sedit $STORE/filter_troff_real $TARGET/filter_troff
	HAVE_TROFF="YES"
	fi
if [ -z "$HAVE_TROFF" ]
	then
	echo "No Troff filter."
	fi

#
# Install DVI filter
#
rm -f $TARGET/filter_dvi		# remove old one
HAVE_DVI=""
if [ -n "$DVIPS" ]				# try dvips
	then
	echo "Installing DVIPS DVI filter."
	sedit $STORE/filter_dvi $TARGET/filter_dvi
	chmod g+s $TARGET/filter_dvi
	HAVE_DVI="YES"
	fi
if [ -z "$HAVE_DVI" ]
	then
	echo "No DVI filter."
	fi

#
# Install TeX filter
# This is only installed if the DVI filter was installed.
#
rm -f $TARGET/filter_tex
HAVE_TEX=""
if [ -n "$HAVE_DVI" -a -n "$TEX" -a -n "$LATEX" ]
	then
	echo "Installing TeX/LaTeX filter."
	sedit $STORE/filter_tex $TARGET/filter_tex
	HAVE_TEX="YES"
	fi
if [ -z "$HAVE_TEX" ]
	then
	echo "No TeX/LaTeX filter."
	fi

#
# Install TexInfo filter
# This is only installed if the DVI filter was installed.
#
rm -f $TARGET/filter_texinfo
HAVE_TEXINFO="YES"
if [ -n "$HAVE_DVI" -a -n "$TEXI2DVI" ]
	then
	echo "Installing TexInfo filter"
	sedit $STORE/filter_texinfo $TARGET/filter_texinfo
	HAVE_TEXINFO="YES"
	fi
if [ -z "$HAVE_TEXINFO" ]
	then
	echo "No Texinfo filter."
	fi

#
# Install PDF filter
#
rm -f $TARGET/filter_pdf
HAVE_PDF=""
if [ -n "$ACROREAD" ]
	then
	echo "Installing PDF filter (Acroread)."
	sedit $STORE/filter_pdf_acroread $TARGET/filter_pdf
	HAVE_PDF="YES"
	fi
if [ -n "$PDFTOPS" ]
	then
	echo "Installing PDF filter (from Xpdf)."
	sedit $STORE/filter_pdf_xpdf $TARGET/filter_pdf
	HAVE_PDF="YES"
	fi
if [ -z "$HAVE_PDF" ]
	then
	echo "No PDF filter"
	fi

#
# Install HTML filter
#
rm -f $TARGET/filter_html
HAVE_HTML=""
if [ -n "$HTMLDOC" ]
	then
	echo "Installing HTML filter (HTMLDOC)."
	sedit $STORE/filter_html_htmldoc $TARGET/filter_html
	HAVE_HTML="YES"
	fi
if [ -z "$HAVE_HTML" ]
	then
	echo "No HTML filter"
	fi

#
# Install JPEG filter
#
rm -f $TARGET/filter_jpeg
HAVE_JPEG=""
if [ -n "$DJPEG" -a -n "$PNMTOPS" ]
	then
	echo "Installing JPEG filter."
	sedit $STORE/filter_jpeg $TARGET/filter_jpeg
	HAVE_JPEG="YES"
	fi
if [ -z "$HAVE_JPEG" ]
	then
	echo "No JPEG filter."
	fi

#
# Install GIF filter
#
rm -f $TARGET/filter_gif
HAVE_GIF=""
if [ -n "$GIFTOPNM" -a -n "$PPMTOPGM" -a -n "$PNMTOPS" ]
	then
	echo "Installing GIF filter."
	sedit $STORE/filter_gif $TARGET/filter_gif
	HAVE_GIF="YES"
	fi
if [ -z "$HAVE_GIF" ]
	then
	echo "No GIF filter."
	fi

#
# Install BMP filter
#
rm -f $TARGET/filter_bmp
HAVE_BMP=""
if [ -n "$BMPTOPPM" -a -n "$PPMTOPGM" -a -n "$PNMTOPS" ]
	then
	echo "Installing BMP filter."
	sedit $STORE/filter_bmp $TARGET/filter_bmp
	HAVE_BMP="YES"
	fi
if [ -z "$HAVE_BMP" ]
	then
	echo "No BMP filter."
	fi

#
# Install PNM filter
#
rm -f $TARGET/filter_pnm
HAVE_PNM=""
if [ -n "$PPMTOPGM" -a -n "$PNMDEPTH" -a -n "$PNMTOPS" ]
	then
	echo "Installing PNM filter."
	sedit $STORE/filter_pnm $TARGET/filter_pnm
	HAVE_PNM="YES"
	fi
if [ -z "$HAVE_PNM" ]
	then
	echo "No PNM filter."
	fi

#
# Install XBM filter
#
rm -f $TARGET/filter_xbm
HAVE_XBM=""
if [ -n "$XBMTOPBM" -a -n "$PNMTOPS" ]
	then
	echo "Installing XBM filter"
	sedit $STORE/filter_xbm $TARGET/filter_xbm
	HAVE_XBM="YES"
	fi
if [ -z "$HAVE_XBM" ]
	then
	echo "No XBM filter."
	fi

#
# Install XPM filter
#
rm -f $TARGET/filter_xpm
HAVE_XPM=""
if [ -n "$XPMTOPPM" -a -n "$PPMTOPGM" -a -n "$PNMDEPTH" -a -n "$PNMTOPS" ]
	then
	echo "Installing XPM filter"
	sedit $STORE/filter_xpm $TARGET/filter_xpm
	HAVE_XPM="YES"
	fi
if [ -z "$HAVE_XPM" ]
	then
	echo "No XPM filter."
	fi

#
# Install XWD filter
#
rm -f $TARGET/filter_xwd
HAVE_XWD=""
if [ -n "$XWDTOPNM" -a -n "$PPMTOPGM" -a -n "$PNMDEPTH" -a -n "$PNMTOPS" ]
	then
	echo "Installing XWD filter"
	sedit $STORE/filter_xwd $TARGET/filter_xwd
	HAVE_XWD="YES"
	fi
if [ -z "$HAVE_XWD" ]
	then
	echo "No XWD filter"
	fi

#
# Install TIFF filter
#
rm -f $TARGET/filter_tiff
HAVE_TIFF=""
if [ -n "$TIFFTOPNM" -a -n "$PPMTOPGM" -a -n "$PNMDEPTH" -a -n "$PNMTOPS" ]
	then
	echo "Installing TIFF filter"
	sedit $STORE/filter_tiff $TARGET/filter_tiff
	HAVE_TIFF="YES"
	fi
if [ -z "$HAVE_TIFF" ]
	then
	echo "No TIFF filter."
	fi

#
# Install PNG filter
#
rm -f $TARGET/filter_png
HAVE_PNG=""
if [ -n "$PNGTOPNM" -a -n "$PPMTOPGM" -a -n "$PNMDEPTH" -a -n "$PNMTOPS" ]
	then
	echo "Installing PNG filter."
	sedit $STORE/filter_png $TARGET/filter_png
	HAVE_PNG="YES"
	fi
if [ -z "$HAVE_PNG" ]
	then
	echo "No PNG filter"
	fi

#
# Install plot filter
#
rm -f $TARGET/filter_plot
HAVE_PLOT=""
if [ -n "$PLOT2PS" ]
	then
	echo "Installing Plot filter (plot2ps)."
	sedit $STORE/filter_plot_plot2ps $TARGET/filter_plot
	HAVE_PLOT="YES"
	fi
if [ -n "$POSTPLOT" ]
	then
	echo "Installing Plot filter (postplot)."
	sedit $STORE/filter_plot_postplot $TARGET/filter_plot
	HAVE_PLOT="YES"
	fi
if [ -z "$HAVE_PLOT" ]
	then
	echo "No Plot filter."
	fi

#
# FIG filter.
#
rm -f $TARGET/filter_fig
HAVE_FIG=""
if [ -n "$FIG2DEV" ]
	then
	echo "Installing FIG filter"
	sedit $STORE/filter_fig $TARGET/filter_fig
	HAVE_FIG="YES"
	fi
if [ -z "$HAVE_FIG" ]
	then
	echo "No FIG filter"
	fi

# we are done
echo
echo "Done."
echo

exit 0

