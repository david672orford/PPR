#! /bin/sh
#
# This command is used to make the gs* series of interfaces
# from gs_master.sh.  We need a separt file because some
# versions of make object to '#' characters.
#
sed -e '/^#/d' -e 's/%/#/g' -e 's/!!$/\\/' -e '/^$/d' -e 's/^\*$//'

