This is mouse.trincoll.edu:~ppr/src/libpprdb/README.txt.

Last updated 6 February 1998.

This directory contains the code for the PPR user database.  The user
database stores user account information including a balance
information.

As far as I know the user database system has never been put into production
because appropriate client side user identification programs for personal
computers have not been written yet.  You should expect bugs if you try to
use this feature.  The user database features, commands, and the functions
in these modules may change as dictated by the demands of actual use.

There are serveral drop-in backend modules for the
PPR user database system.  These are the modules
which are currently provided:

gdbm	Implements the PPR user database using the GNU database library.

none	A dummy implementation which contains functions which produce
	an error message and report failure.

The Makefile in this directory compiles one of these systems and gathers
it into libpprdb.a.  The one to be compiled is chosen with the make
variable "USER_DBM" which is defined in global.mk.

=== end of file ===
