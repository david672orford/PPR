mouse:~ppr/src/nonppr_misc/README.txt
Last modified 4 March 2003.

This directory contains small bits of code and documents which are not under 
the PPR license.  There are symbolic links in other parts of the source tree 
which point to these files.  The files are listed below according to the names
of their symbolic links.

Please see ../LICENSE.txt for a list of other directories containing
material not under the PPR license.

libgu/gu_md5.c -> nonppr_misc/md5/gu_md5.c
include/gu_md5.h -> nonppr_misc/md5/gu_md5.h
	Copyright 1999, Aladdin Enterprises, BSDish license described in
	the files themselves.

www/md5.js -> nonppr_misc/md5/md5.js
	Copyright (c) 1996 Henri Torgemane, BSDish license described in 
	the file.

pprppopup/md5pure.tcl -> nonppr_misc/md5/md5pure.tcl
	Copyright 1999, Don Libes

libscript/MD5pp.pm -> nonppr_misc/md5/MD5pp.pm
	More info needed.

libgu/snprintf.c -> nonppr_misc/snprintf/snprintf.c
	Copyright 1995, Patrick Powell, preservation of copyright notice in
	source code required.

makeprogs/squeeze.c -> nonppr_misc/squeeze/squeeze.c
	Copyright 1988, Radical Eye Software.  This program is a modified
	version of squeeze.c from the DVIPS distribution.

libgu/gu_wildmat.c -> nonppr_misc/wildmat/gu_wildmat.c
nonppr_misc/wildmat/gu_wildmat.3 (no symbolic link)
	Placed in the public domain by the author, Rich $alz.  Copyright on
	modifications for PPR is likewise abandoned.  Look in gu_wildmat.c
	for a list of changes.

libttf/ps_type3.c -> nonppr_misc/ps_type3/ps_type3.c
	The code in this file is almost all from L. S. Ng's
	comp.sources.postscript USENET posting of a program to create
	PostScript from a TrueType font file and a text file.

include/cexcept.h -> nonppr_misc/cexcept-1.0.0/cexcept.h
	Written by Adam M. Costello.  See nonppr_misc/cexcept-1.0.0/README in for
	license terms.

docbook-xml -> docbook-xml-4.2
	This is the DocBook XML 4.2 DTD and the entities it needs.  It is
	used to format the PPR documentation.  It is downloaded from
	<http://www.docbook.org/xml/4.2/docbook-xml-4.2.zip>.  It is
	excluded from the PPR tarball since the tarball has pre-built
	documentation.

docbook-xsl ->
	This should point at the DocBook XSL stylesheets which may be obtained
	from <http://docbook.sourceforge.net/projects/xsl/>.  They are
	rather large, so they aren't in PPR's CVS at the moment and are
	excluded from the tarball.

If you discover any errors or omisions in this list of exceptions to the
PPR license, please write to <ppr-bugs@mail.trincoll.edu>.
