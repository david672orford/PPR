#
# mouse:~ppr/src/libscript/acl.pl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 11 January 2001.
#

require 'paths.ph';

my @internal_list = (
	"root",
	$USER_PPR,
	$USER_PPRWWW
	);

sub user_acl_allows
    {
    my($user, $acl) = @_;

    # Look in one of the internal lists.  These lists are compiled in
    # because the system will cease to function correctly if these usernames
    # are removed.
    foreach my $i (@internal_list)
    	{
	return 1 if($user eq $i);
    	}

    # Look for a line with the user name in the .allow
    # file for the ACL list.
    if(open(ACL_FILE, "<$ACLDIR/$acl.allow"))
	{
	my $line;
	while($line = <ACL_FILE>)
	    {
	    # Skip comments.
	    next if($line =~ /^#/ || $line =~ /^;/);

	    # Trim trailing whitespace.
	    $line =~ s/\s+$//;

	    # Does it match?
	    if($line eq $user)
	    	{
	    	close(ACL_FILE);
	    	return 1;
	    	}
	    }
	close(ACL_FILE);
	}

    return 0;
    }

# end of file
