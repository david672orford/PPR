#! ../nonppr_tcl/ppr-tclsh
#
# mouse:~ppr/src/z_install_end/install_alternatives.sh
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 15 March 2003.
#

source ../makeprogs/paths.tcl

set list {
	{lpr 1}
	{lpq 1}
	{lprm 1}
	{lp 1}
	{lpstat 1}
	{cancel 1}
	}

if {[file isdirectory /etc/alternatives] && [file executable /usr/sbin/update-alternatives]} {
    puts "  Registering PPR programs with alternatives system..."
    foreach i $list {
	puts "    $i"
	set prog [lindex $i 0]
	set mans [lindex $i 1]
	puts "/usr/sbin/update-alternatives
		--install /usr/bin/$prog
			$prog
			$HOMEDIR/bin/uprint-$prog
			9
		--slave /usr/share/man/man$mans/$prog.$mans
			man-${prog}
			$SHAREDIR/man/man$mans/uprint-${prog}.$mans"
	}
    }

exit 0
