#! /usr/bin/perl
#
# mouse:~ppr/src/filters_misc/tex.perl
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
# Last modified 24 August 2005.
#

#
# This filter takes Plain TeX or LaTeX source and passes it thru
# TeX or LaTeX and then passes it on the the DVI filter.
#
# The program indexfilters passes this program thru a sed script before 
# installing it.
#
$TEMPDIR="@TEMPDIR@";
$FILTDIR="@FILTDIR@";
$TEX="@TEX@";
$LATEX="@LATEX@";

# Fool Perl so it will not complain about
# our already secure PATH and IFS.
$ENV{'PATH'} =~ /^(.*)$/;
$ENV{'PATH'}=$1;
$ENV{'IFS'} =~ /^(.*)$/;
$ENV{'IFS'}=$1;

#
# Function to spawn a process and wait for it.
# The first argument is the file to connect to
# the child's stdin, the second is the file to run as
# the child.
#
sub run_with_stdin
	{
	local($pid);
	local($infile,$progname) = @_;

	$pid = fork();

	if($pid == -1)						# error
		{								# not much we can do
		die "Fork() failed\n";
		}
	elsif($pid == 0)					# child
		{								# exec the program
		open(STDIN,"<$infile") || die "child: failed to open $infile as stdin, $!\n";
		exec($progname);
		exit(242);
		}
	else								# parent
		{								# wait for child to exit
		wait;
		if($? != 0)
			{
			die "Failure while runing $progname, \$? = $?\n";
			}
		}
	} # end of run()

# We need a directory to work in
$TEXTEMPDIR="$TEMPDIR/ppr-tex-$$";
umask(0);
# I can't remember why this has to be 0755:
mkdir($TEXTEMPDIR,0755) || die "Failed to make directory $TEXTEMPDIR\n";

# name the parameters
($options, $printer, $title) = @ARGV;

# Things to count
$count_begin=0;
$count_end=0;

# Flag which indicates if we should print lots of messages
$noisy=0;

# Read the parameters
foreach $pair (split(/[ \t]+/,$options))
	{
	if( $pair =~ /^noisy=[ty1]/i )
		{
		$noisy=1;
		}
	}

# Read in the TeX source, analyzing it as we go
print STDERR "Reading and analyzing document\n" if $noisy;
open(TEMPFILE,"> $TEXTEMPDIR/document.tex") || die "Failed to create \"$TEXTEMPDIR/document.tex\", $!\n";
while(<STDIN>)			# Read all of the TeX source
		{				# and copy it into a temporary file
		if( /\\begin *{[^}]+}/ )
			{ $count_begin++; }

		elsif( /\\end *{[^}]+}/ )
			{ $count_end++; }

		print TEMPFILE $_;
		}
close(TEMPFILE);

# See if \begin{} and \end{} are mismatched
if( $count_begin && $count_begin != $count_end)
	{
	print STDERR "\\begin{} \\end{} mismatch\n" if $noisy;

	open(E,"> $TEXTEMPDIR/error") || die "Can't create $TEXTEMPDIR/error\n";

	print E "Your LaTeX job was not submitted to LaTeX because a preliminary\n";
	print E "inspection revealed that while it has $count_begin \\begin{}\n";
	print E "commands, its has $count_end \\end{} commands.\n";

	close(E);

	&run_with_stdin("$TEXTEMPDIR/error", "$FILTDIR/filter_lp");
	unlink("$TEXTEMPDIR/error");
	unlink("$TEXTEMPDIR/document.tex");
	rmdir($TEXTEMPDIR);
	exit(0);			# In a sense, we were sucessfull
	}

# Decide which program to use
if($count_begin)
	{
	$PROGRAM=$LATEX;
	}
else
	{
	$PROGRAM=$TEX;
	}

# Now, run it thru TeX or LaTeX
for($times_run=0,$run_needed=1; $run_needed && $times_run < 3; $times_run++)
	{
	$run_needed=0;								# assume this run will do it
	$texerror=0;

	print STDERR "Running \"$PROGRAM\"\n" if $noisy;

	pipe(TEX,CHILD) || die "pipe() failed, $!\n";
	$pid=fork();

	if(!defined($pid))
		{ die "fork() failed, $!\n"; }

	if($pid==0)									# child
		{
		close(TEX);
		open(STDOUT,">&CHILD");
		open(STDERR,">&STDOUT");				# direct stderr to pipe too
		close(CHILD);
		chdir($TEXTEMPDIR);						# Let TeX work in temporary directory
		exec($PROGRAM,"document.tex");
		exit(255);
		}

	close(CHILD);								# parent doesn't need this

	$texoutput="";
	while(<TEX>)
		{
		$texoutput .= $_;						# Save it

		print STDERR $_ if $noisy;				# If running noisy, let user see it

		if( /^Overfull/ || /^Underfull/ )		# ignore lines which may
			{ next; }							# contain source text

		if( /Rerun/ )							# note any lines which say
			{ $run_needed=1; }					# TeX should be run again

		if( /^! Emergency stop/ )
			{ $texerror=1; }
		}

	close(TEX);			# close the pipe from TeX
	wait();				# wait for TeX to die
	$retval=$?;			# save TeX's return value

	if($retval == 255)
		{
		die "exec($PROGRAM) failed, $!\n";
		}

	# If TeX reported errors, print TeX's output
	# instead of the file.
	elsif($retval != 0 || $texerror)
		{
		print STDERR "Printing errors\n" if $noisy;

		open(ERR,">$TEXTEMPDIR/error") || die "Failed to create temporary file, $!\n";
		print ERR $texoutput;
		close(ERR);

		&run_with_stdin("$TEXTEMPDIR/error", "$FILTDIR/filter_lp");

		last;
		}
	} # end of for() loop which runs TeX or LaTeX multiple times

# If TeX suceeded, pass the file to the dvi filter
if($retval == 0 && $texerror == 0)
	{
	print "Running filter_dvi\n" if $noisy;

	if( ! defined($pid=fork()) )
		{ die "Can't fork()\n"; }

	if($pid==0)			# child
		{
		open(STDIN,"< $TEXTEMPDIR/document.dvi") || die "failed to open $TEXTEMPDIR/document.dvi\n";
		exec("$FILTDIR/filter_dvi", $options, $printer, $title);
		die "exec(\"$FILTER_DVI\") failed, $!\n";
		}
	else				# parent
		{
		wait();
		}
	}

# remove all the temporary files
print STDERR "Removing temporary files:\n" if $noisy;
chdir($TEXTEMPDIR);
opendir(DIR,".") || die "opendir() failed\n";
while(defined($file=readdir(DIR)))
	{
	if($file !~ /^\./)
		{
		print STDERR "\t$file\n" if $noisy;
		# Workaround for new Perl paranoia:
		$file =~ /^(.*)$/;
		unlink($1);
		}
	}
closedir(DIR);
chdir("..");
rmdir($TEXTEMPDIR);

# we are done
print "Done\n" if $noisy;
exit 0;
