#
# mouse:~ppr/src/printdesk/PrintDesk.pm
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
# Last modified 17 January 2005.
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
$BITMAPS = "${PPR::PERL_LIBDIR}/lib/PrintDesk";
$GETZONES = "${PPR::LIBDIR}/lib/getzones";
$NBP_LOOKUP = "${PPR::LIBDIR}/lib/nbp_lookup";
$PPOP_PATH = $PPR::PPOP_PATH;
$TAIL_STATUS_PATH = $PPR::TAIL_STATUS_PATH;

$PrintDesk::BatchDialog::default_width = 400;
$PrintDesk::BatchDialog::default_font = "-*-times-medium-r-*-*-*-140-*-*-*-*-*-*";

1;
