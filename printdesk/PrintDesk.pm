#
# mouse:~ppr/src/printdesk/PrintDesk.pm.perl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last revised 8 May 2002.
#

#
# This top level class contains information needed by
# the lower level classes.  A lot of this stuff should
# be defined in an easier-to-find place.
#

package PrintDesk;

# This is the PrintDesk version number.
$VERSION = 0.2;

# Pull in the PPR path information.
use PPR;

# Where do we find files and programs we need?
$BITMAPS = "${PPR::HOMEDIR}/lib/PrintDesk";
$GETZONES = "${PPR::HOMEDIR}/lib/getzones";
$NBP_LOOKUP = "${PPR::HOMEDIR}/lib/nbp_lookup";
$PPOP_PATH = $PPR::PPOP_PATH;
$TAIL_STATUS_PATH = $PPR::TAIL_STATUS_PATH;

$PrintDesk::BatchDialog::default_width = 400;
$PrintDesk::BatchDialog::default_font = "-*-times-medium-r-*-*-*-140-*-*-*-*-*-*";

1;
