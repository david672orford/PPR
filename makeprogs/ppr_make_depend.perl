#! /usr/bin/perl
#
# mouse:~ppr/makeprogs/ppr_make_depend.perl
#
# Last modified 23 September 2005.
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

		print OUT "$1.o: ${SEARCHDIR}${file}";

		while(<F>)
				{
				if(/^#include "([^"]+)"/)
						{
						my $incfile = $1;
						if($incfile =~ /\// || -f $incfile)
							{ print OUT " $incfile" }
						elsif(-f "$INCDIR/$incfile")
							{ print OUT " $INCDIR/$incfile" }
						}
				}

		print OUT "\n\n";

		close(F);
		}

closedir(D);

close(OUT);

exit(0);

