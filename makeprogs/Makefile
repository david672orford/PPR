#
# mouse:~ppr/src/makeprogs/Makefile
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 21 June 2001.
#

include ../makeprogs/global.mk

TARGETS=gen_paths$(DOTEXE) paths.ph paths.mk paths.sh PPR.pm squeeze$(DOTEXE)

all: $(TARGETS)

install: all
	$(RMF) installed_files_list
	./make_install_dirs.sh
	$(INSTALLDATA) $(HOMEDIR)/lib paths.ph PPR.pm

include .depend

paths.ph: gen_paths$(DOTEXE)
	./gen_paths --ph >paths.ph

PPR.pm: gen_paths$(DOTEXE)
	./gen_paths --pm >PPR.pm

paths.mk: gen_paths$(DOTEXE)
	./gen_paths --mk >paths.mk

paths.sh: gen_paths$(DOTEXE)
	./gen_paths --sh >paths.sh

depend:
	$(PPR_MAKE_DEPEND) ../include

# Remove backup files, targets, and replace paths.mk with an empty file 
# so that we can run "make clean".
clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(TARGETS)
	$(RMF) installed_files_list
	touch paths.mk

# end of file

