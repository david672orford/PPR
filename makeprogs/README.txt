mouse:~ppr/src/makeprogs/README.txt
2 August 2003

This directory contains programs used to build PPR.  The install*.sh programs
contain code to copy files and set their permissions while printing verbose
messages.  They mind $RPM_BUILD_ROOT so that the makefiles needn't.

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
	deletes a directory, if it exists, and creates it anew as specified

ppr-config.c
	prints PPR configuration information

readlink.c
	A basic implementation of readlink(1).  We need this because some
	systems don't have one even though they support symbolic links!

save_symlinks.sh
	Called by ../Makefile, this creates ../.symlinks-restore.  This is
	all because CVS can't store symbolic links.

scriptfixup.sh
	copies scripts while inserting program and user names

squeeze.c
	This filter that removes syntactically meaningless whitespace from 
	PostScript code.  This means that we can have lots of whitespace 
	and comments in procedure sets without wasting bandwidth on the
	link to the printer.

fixed_cc_osf.sh
	This is a compiler wrapper script which filters source files to remove
	any horizontal whitespace which might procede compiler directives. 
	One wonders what could have motivated anyone to write a compiler
	that way.

ppr_make_depend.perl
	This script searches the C source files mentioned on the command
	line and generates a list of files which it includes (excluding
	system includes) and generates a .depend file in the current
	directory.

version.mk
	This tiny file ontains the current version number, is updated
	automatically, is used by ../Makefile to figure out what it should
	call the source tarball when doing "make dist".

End of README.txt.
