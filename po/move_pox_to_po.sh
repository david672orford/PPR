#! /bin/sh

for i in PPR PPRD PPRDRV PPRWWW PAPSRV
    do
    mv $1-$i.pox $1-$i.po
    done

exit 0
