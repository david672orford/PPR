mouse:~ppr/src/unixuser/README.txt
13 February 2001


======================
 ppr-xfm.sh
 ppr-xgrant.c
======================

These programs are described in the ppr-xfm(1) and ppr-xgrant(1) man pages.
They are installed in $HOMEDIR/bin/ by "make install".


=========================
 pprpopupd.perl
=========================

This is a daemon which may be run out of Inetd to receive popup messages
from the pprpopup responder.  It is installed in $HOMEDIR/lib/ by
"make install".

This script allows Unix workstations to receive popup responses and
commentator output from PPR just as MS-Windows workstations can received
them if they are running ../samba/pprpopup.tcl.

To run this program, add this line to /etc/services:

pprpopup	15009/tcp

And this one to /etc/inetd.conf:

pprpopup stream tcp nowait ppr /usr/lib/ppr/lib/pprpopupd pprpopupd

Or, if you are using TCP wrappers (as you should):

pprpopup stream tcp nowait ppr /usr/sbin/tcpd /usr/lib/ppr/lib/pprpopupd

The messages are delivered by xmessage if you recipient owns /dev/console,
otherwise they are delivered by write.  PPR will only be able to open an
xmessage window on the display if the currenly logged in user has run
ppr-xgrant.  Here are the commands to encourage PPR to deliver its response
messages to pprpopupd:

/usr/lib/ppr/bin/ppr-xgrant
PPR_RESPONDER=pprpopup
PPR_RESPONDER_ADDRESS=$LOGNAME@`uname -n`:15009
export PPR_RESPONDER PPR_RESPONDER_ADDRESS


=========================
End of file

