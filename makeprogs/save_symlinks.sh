#! /bin/sh
#
# mouse:~ppr/src/makeprogs/save_symlinks.sh
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
# Last modified 28 March 2002.
#

exec >.restore_symlinks
chmod 755 .restore_symlinks
echo "#! /bin/bash"

for link in `find . -type l`
	do
	contents=`readlink $link`

	# -h doesn't seem to work with some versions of bash.
	#echo "if [ ! -h $link ]; then ln -sf $contents $link || exit 1; fi"
	echo "if [ ! -f $link ]; then ln -sf $contents $link || exit 1; fi"

	done

exit 0
