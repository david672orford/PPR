#! /usr/bin/perl
#
# mouse:~ppr/makeprogs/ppr_make_depend.perl
#
# Last modified 10 May 2001.
#

defined($INCDIR = $ARGV[0]) || die;

if(defined($SEARCHDIR = $ARGV[1]))
	{ $SEARCHDIR .= '/' }
else
	{ $SEARCHDIR = '' }

opendir(D, $SEARCHDIR ne "" ? $SEARCHDIR : ".") || die;

open(OUT, ">${SEARCHDIR}.depend") || die;

while($file = readdir(D))
	{
	next if($file !~ /^([^\.]+)\.c$/);

	print "Examining $file\n";

	open(F, "<${SEARCHDIR}${file}") || die;

	print OUT "$1.\$(OBJ): ${SEARCHDIR}${file}";

	while(<F>)
		{
		if(/^#include "([^"]+)"\s*$/)
			{
			my $incfile = $1;
			if($incfile =~ /\// || -f $incfile)
			    { print OUT " $incfile" }
			else
			    { print OUT " $INCDIR/$incfile" }
			}
		}

	print OUT "\n\n";

	close(F);
	}

closedir(D);

close(OUT);

exit(0);

