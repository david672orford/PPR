#! /bin/sh

# This is a temporary implementation.  It uses the htdigest program from
# the Apache web server.

CONFDIR="?"
USER_PPR=?
GROUP_PPR=?

if [ "$1" = "" ]
    then
    echo "Usage: ppr-passwd <username>"
    exit 1
    fi

if [ ! -f $CONFDIR/htpasswd ]
    then
    touch $CONFDIR/htpasswd
    chmod 660 $CONFDIR/htpasswd
    chown $USER_PPR $CONFDIR/htpasswd
    chgrp $GROUP_PPR $CONFDIR/htpasswd
    fi

# For Solaris 8
PATH=$PATH:/usr/apache/bin
export PATH

htdigest $CONFDIR/htpasswd "printing" $1

exit $?

