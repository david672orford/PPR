#
# mouse:~ppr/src/makeprogs/global.mk.unconfigured
#
# If you run "make unconfigure" in the top level PPR source directory, this
# file is copied to makeprogs/global.mk.
#
# The Configure script creates a new global.mk, replacing the one which was
# copied from this file.  The idea of this include file is to prevent anything
# but "make clean" from running.
#

CP=cp
RMF=rm -f
BACKUPS=*~ *.bak *.bck
PPR_MAKE_DEPEND=../makeprogs/ppr_make_depend.perl

# This is first, so it will be the default target.
cant:
	@echo "You must run Configure before you can run make."
	@exit 1

# Catch "make all".
all: cant

# Catch "make install".
install: cant

# end of file

