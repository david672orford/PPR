#! /bin/sh

if [ "$1" = "" ]
	then
	echo "Usage: move_pox_to_po.sh <language>"
	exit 1
	fi

for i in PPR PPRD PPRDRV PPRWWW PAPSRV PAPD
	do
	mv $1-$i.pox $1-$i.po
	done

exit 0
