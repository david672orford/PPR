#! /bin/sh
#
# mouse:~ppr/src/fixed_cc_osf.sh
# Copyright 1996, 1998, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 17 September 1998.
#

#
# This shell script is a wrapper for the DEC OSF C compiler.  This compiler
# manifests a couple bizzar features.  The strangest is that C preprocessor
# directives must all begin in column 1.  This script runs each source
# file thru sed to work around the bug.	 (Incidentally, this behavior is
# documented in the cc(1) man page, so I guess that makes it a feature.)
#
# This script is also useful with the ULTRIX C compiler.
#

# A temporary file which will be used to
# store the modified source code:
TEMP_FILE_BASE="fixed_cc_$$"
TEMPFILE="/tmp/$TEMP_FILE_BASE.c"

# The start of the command line we are building.  We include "-I."
# because the C compiler looks for include files in the same directory
# as the file being compiled.  Since we are compiling a file in the /tmp
# directory we must explicitly include the origional source directory
# in the include search path.
command="cc -I."

# Examine each argument.  When we see a file name ending in ".c" we
# will filter it with sed and replace its name with the name of the
# sed output file.
#
# If the last argument was an "-I" switch, don't leave space between
# it and the present argument since the OSF C compiler has wierd
# argument parsing.
for i in $*
  do
  if echo $i | grep '\.c$' >/dev/null
	then
	sed -e 's/^[	][	]*#/#/' $i >$TEMPFILE
	command="$command $TEMPFILE"
	module_name=`echo $i | sed -e 's/\.c$//'`
	else
	if [ "$last_i" = "-I" ]
	  then
	  command="$command$i"
	  else
	  command="$command $i"
	  fi
	fi
  last_i=$i
  done

# Fix quotes in the command since the extra shell evaluations have
# mangled them:
command=`echo "$command" | sed -e 's/["]/\\\\"/g'`

# Run the C compiler:
echo "$command"
eval $command
RESULT=$?

# Remove the sed output file:
rm -f $TEMPFILE

# If the compiler generated an output file it generated it with a
# name based uppon the name of the sed output file.	 Rename the
# file to the name it would have had if the source file had been
# compiled directly.
if [ -f $TEMP_FILE_BASE.o ]
  then
  echo "mv $TEMP_FILE_BASE.o $module_name.o"
  mv $TEMP_FILE_BASE.o $module_name.o
  fi

# Try to pass the C compiler exit code to make:
exit $RESULT

