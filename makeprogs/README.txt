mouse:~ppr/src/makeprogs/README.txt
11 March 2003

This directory contains programs used to build PPR.

installconf.sh
	sets the permission of an already installed configuration file and enters
	it file in the list for RPM

installcp.sh
	copies a file and enters it in the RPM list

installdata.sh
	copies one or more data file into an indicated directory and enters
	them in the RPM list

installln.sh
	creates a symbolic link if it doesn't already exist and enters it
	into the RPM list

installprogs.sh
	installs programs, strips them, and enters them in the RPM list

make_new_dir.sh
	deletes a directory if it exists and creates it new as specified

ppr-config.c
	prints PPR configuration information

readlink.c
	basic implementation of readlink(1)

save_symlinks.sh
	create ../.symlinks-restore

scriptfixup.sh
	copies scripts while inserting program and user names

squeeze.c
	filter that removes syntactically meaningless whitespace from 
	PostScript code

fixed_cc_osf.sh
	compiler wrapper which filters source files to remove whitespace 
	before compiler directives

ppr_make_depend.perl
	generates a .depend file

version.mk
	contains the current version number, is updated automatically, is
	used by ../Makefile


