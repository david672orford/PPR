#
# mouse:~ppr/src/filter_pcl/Makefile
# Last revised 22 November 2000.
#

include ../makeprogs/global.mk

PROGS=filter_pcl

OBJS=filter_pcl.class PCL.class PSout.class AssertionFailed.class

all: $(PROGS)

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/filters $(PROGS)

filter_pcl: $(OBJS)
	gcj -o filter_pcl --main=filter_pcl $(OBJS)

.SUFFIXES: .java .class

.java.class:
	gcj -C $*.java

depend:

clean:
	$(RMF) $(BACKUPS) $(OBJS) $(PROGS)

