#if 0
% mouse:~ppr/src/interfaces/gs_master.sh
%
% After this file is passed thru cpp it is passed thru sed which
% changes all %'s to #'s and changes all instances of !! at the
% end of a line to backslash, deletes all blank lines converts lines
% which contain only "*" to blank lines.
%
#endif

%! /bin/sh
%
#if defined(BACKEND_SIMPLE)
% mouse:~ppr/src/interfaces/gssimple.sh
#elif defined(BACKEND_PARALLEL)
% mouse:~ppr/src/interfaces/gsparallel.sh
#elif defined(BACKEND_SERIAL)
% mouse:~ppr/src/interfaces/gsserial.sh
#elif defined(BACKEND_ATALK)
% mouse:~ppr/src/interfaces/gsatalk.sh
#elif defined(BACKEND_TCPIP)
% mouse:~ppr/src/interfaces/gstcpip.sh
#elif defined(BACKEND_LPR)
% mouse:~ppr/src/interfaces/gslpr.sh
#elif defined(BACKEND_SMB)
% mouse:~ppr/src/interfaces/gssmb.sh
#else
#error File name comment missing
#endif

% Copyright 1995--2001, Trinity College Computing Center.
% Written by David Chappell.
%
% Permission to use, copy, modify, and distribute this software and its
% documentation for any purpose and without fee is hereby granted, provided
% that the above copyright notice appear in all copies and that both that
% copyright notice and this permission notice appear in supporting
% documentation.  This software is provided "as is" without express or
% implied warranty.
%
% Last modified 5 April 2001.
%
*

%
% This file was automatically generated from:
% mouse:~ppr/src/interfaces/gs_master.sh
%
% This interface script passes the file through GhostScript.
#if defined(BACKEND_SIMPLE)
% Ghostscript is instructed to use the file or port specified
% in the address as its output file.
#elif defined(BACKEND_PARALLEL)
% The output of Ghostscript is sent to the printer by running
% the parallel interface as a child process of GhostScript.
#elif defined(BACKEND_SERIAL)
% The output of Ghostscript is sent to the printer by running
% the serial interface as a child process of GhostScript.
#elif defined(BACKEND_ATALK)
% The output of Ghostscript is sent to the printer by running
% the atalk interface as a child process of GhostScript.
#elif defined(BACKEND_TCPIP)
% The output of Ghostscript is sent to the printer by running
% the tcpip interface as a child process of GhostScript.
#elif defined(BACKEND_LPR)
% The output of Ghostscript is sent to the printer by running
% the lpr interface as a child process of GhostScript.
#elif defined(BACKEND_SMB)
% The output of Ghostscript is sent tothe printer by running
% the smb interface as a child process of Ghostscript.
#else
#error Description missing
#endif
%
*

% This value will be filled in
% by scriptfixup.sh:
SHAREDIR="?"
*

% Where is GhostScript?  This is a default value.
% It can be changed by setting the gs= interface
% option for the printer.
GS=`lib/ppr_conf_query ghostscript gs 0 /usr/bin/gs`
*

% give the parameters names
OPT_PRINTER="$1"	% name of printer
OPT_ADDRESS="$2"	% address
OPT_OPTIONS="$3"	% options
OPT_JOBNAME="$7"	% name of the job
shift 1
OPT_BARBARLANG="$9"
*

% Read interface.sh so we will know
% the correct exit codes.
. lib/interface.sh
*

#ifndef BACKEND_SIMPLE
% Read the signal numbers.
. lib/signal.sh
*
#endif
#

% Parse the options.
#ifndef BACKEND_SIMPLE
% Save unknown options for the back end interface.
#endif
DEVICE=""
UNIPRINT=""
RESOLUTION=""
#ifndef BACKEND_SIMPLE
#ifdef BACKEND_ATALK
PPR_GS_INTERFACE_HACK_OPTIONS="is_laserwriter=false"
#else
PPR_GS_INTERFACE_HACK_OPTIONS=""
#endif
#endif
GSOPTS=""
for opt in $OPT_OPTIONS
    do
    case $opt in
	device=* )
	    DEVICE=`echo $opt | cut -d'=' -f2`
	    ;;
	uniprint=* )
	    DEVICE="uniprint"
	    UNIPRINT="@`echo $opt | cut -d'=' -f2`.upp"
	    ;;
	resolution=* )
	    RESOLUTION="-r`echo $opt | cut -d'=' -f2`"
	    ;;
	gs=* )
	    GS=`echo $opt | cut -d'=' -f2`
	    ;;
	gsopt=* )
	    GSOPTS="$GSOPTS `echo $opt | cut -d'=' -f2-`"
	    ;;
	* )
#ifdef BACKEND_SIMPLE
	    lib/alert $OPT_PRINTER TRUE "Unrecognized interface option: $opt"
	    exit $EXIT_PRNERR_NORETRY
#else
	    if [ "$PPR_GS_INTERFACE_HACK_OPTIONS" = "" ]
	        then
		PPR_GS_INTERFACE_HACK_OPTIONS="$opt"
		else
		PPR_GS_INTERFACE_HACK_OPTIONS="$PPR_GS_INTERFACE_HACK_OPTIONS $opt"
		fi
#endif
	    ;;
    esac
    done
*

% make sure we have an option defined
if [ -z "$DEVICE" ]
	then
	lib/alert $OPT_PRINTER TRUE "Options must include either \"device=\" or \"uniprint=\" to set the"
	lib/alert $OPT_PRINTER FALSE "Ghostscript device driver name or Uniprint minidriver."
	exit $EXIT_PRNERR_NORETRY
	fi
*

#ifdef BACKEND_SIMPLE
% make sure the port can be written to
if [ ! -w "$OPT_ADDRESS" ]
	then
	if [ -c "$OPT_ADDRESS" ]
		then
		lib/alert $OPT_PRINTER TRUE "No permission to open port \"$OPT_ADDRESS\"."
		else
		lib/alert $OPT_PRINTER TRUE "Port \"$OPT_ADDRESS\" does not exist."
		fi
	exit $EXIT_PRNERR_NORETRY
	fi
*
#endif

% make sure we have GhostScript
if [ ! -x "$GS" ]
	then
	lib/alert $OPT_PRINTER TRUE "Path to GhostScript (\"$GS\") is incorrect."
	lib/alert $OPT_PRINTER FALSE "Use the gs= option to specify the correct path."
	exit $EXIT_PRNERR_NORETRY
	fi
*

#ifndef BACKEND_SIMPLE
% Set up signal handlers to catch signals from
% the back-end interface:
sub_retval=$EXIT_PRINTED
trap 'sub_retval=$EXIT_PRNERR' $SIGUSR1
trap 'sub_retval=$EXIT_PRNERR_NORETRY' $SIGUSR2
trap 'sub_retval=$EXIT_ENGAGED' $SIGTTIN
*
#endif

#ifndef BACKEND_SIMPLE
% Export the options we did not consume and the printer address as environment
% variables because Ghostscript was a short output program command line limit.
export PPR_GS_INTERFACE_HACK_OPTIONS
PPR_GS_INTERFACE_HACK_ADDRESS="$OPT_ADDRESS"
export PPR_GS_INTERFACE_HACK_ADDRESS
*
#endif

% If this is not a PostScript job, then replace this process with
% the subsidiary interface.
if [ -n "$OPT_BARBARLANG" ]
then
#if defined(BACKEND_SIMPLE)
cat - >$OPT_ADDRESS
#elif defined(BACKEND_PARALLEL)
exec interfaces/parallel $OPT_PRINTER '' '' $JOBBREAK_NONE 0
#elif defined(BACKEND_SERIAL)
exec interfaces/serial $OPT_PRINTER '' '' $JOBBREAK_NONE 0
#elif defined(BACKEND_ATALK)
exec interfaces/atalk $OPT_PRINTER '' '' $JOBBREAK_NONE 0
#elif defined(BACKEND_TCPIP)
exec interfaces/tcpip $OPT_PRINTER '' '' $JOBBREAK_NONE 0
#elif defined(BACKEND_LPR)
exec interfaces/lpr $OPT_PRINTER '' '' $JOBBREAK_NONE 0
#elif defined(BACKEND_SMB)
exec interfaces/smb $OPT_PRINTER '' '' $JOBBREAK_NONE 0
#else
#error Missing Ghostscript arguments section in gs_master.sh
#endif
#ifndef BACKEND_SIMPLE
lib/alert $OPT_PRINTER TRUE "Exec of subsidiary interface failed."
exit $EXIT_PRNERR_NORETRY
#endif
fi
*

% If we get this far, it is a PostScript job, run Ghostscript.
#ifndef BACKEND_SIMPLE
PPR_GS_INTERFACE_PID=$$
export PPR_GS_INTERFACE_PID
#endif
$GS -q -dSAFER -sDEVICE=$DEVICE $RESOLUTION $UNIPRINT !!
#if defined(BACKEND_SIMPLE)
  -sOutputFile=$OPT_ADDRESS $GSOPTS -
#elif defined(BACKEND_PARALLEL)
  -sOutputFile="|interfaces/parallel $OPT_PRINTER '' '' $JOBBREAK_NONE 0" $GSOPTS -
#elif defined(BACKEND_SERIAL)
  -sOutputFile="|interfaces/serial $OPT_PRINTER '' '' $JOBBREAK_NONE 0" $GSOPTS -
#elif defined(BACKEND_ATALK)
  -sOutputFile="|interfaces/atalk $OPT_PRINTER '' '' $JOBBREAK_NONE 0" $GSOPTS -
#elif defined(BACKEND_TCPIP)
  -sOutputFile="|interfaces/tcpip $OPT_PRINTER '' '' $JOBBREAK_NONE 0" $GSOPTS -
#elif defined(BACKEND_LPR)
  -sOutputFile="|interfaces/lpr $OPT_PRINTER '' '' $JOBBREAK_NONE 0" $GSOPTS -
#elif defined(BACKEND_SMB)
  -sOutputFile="|interfaces/smb $OPT_PRINTER '' '' $JOBBREAK_NONE 0" $GSOPTS -
#else
#error Missing Ghostscript arguments section
#endif
*

% Save the Ghostscript return value before it is lost.
retval=$?
*

#ifndef BACKEND_SIMPLE
% If the interface sent a signal around Ghostscript, then exit with the
% code that cooresponds to that signal.
if [ $sub_retval -ne $EXIT_PRINTED ]	% if signal handler changed sub_retval,
    then
    exit $sub_retval
    fi
*
#endif

% If we get here, the interface didn't report failure.  Let's see
% what Ghostscript had to say.
case $retval in
    0 )				% No error
	exit $EXIT_PRINTED
	;;
    1 )				% Syntax error?
	lib/alert $OPT_PRINTER TRUE "Possible error on Ghostscript command line.  Check options."
	lib/alert $OPT_PRINTER FALSE "(Use \"ppop log $OPT_JOBNAME\" to view the error message.)"
	exit $EXIT_PRNERR_NORETRY
	;;
    139 )			% Seems to indicate a core dump
	lib/alert $OPT_PRINTER TRUE "Ghostscript may have dumped core."
	exit $EXIT_JOBERR
	;;
    141 )			% Broken pipe, presumable subsidiary interface exited.
	lib/alert $OPT_PRINTER TRUE "The subsidiary interface may have failed."
	exit $EXIT_PRNERR
	;;
    243 | 255 )			% PostScript error?
	exit $EXIT_JOBERR
	;;
    * )
	lib/alert $OPT_PRINTER TRUE "GhostScript returned unrecognized error code $retval."
	lib/alert $OPT_PRINTER FALSE "(Use \"ppop log $OPT_JOBNAME\" to view the error message.)".
	exit $EXIT_PRNERR
	;;
esac
*

% end of file

