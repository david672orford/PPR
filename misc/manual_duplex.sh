#! /bin/sh
PRINTER=$1
FILE="$2"
ppr -d $PRINTER -p 'odd=yes even=no' "$FILE"
ppr -d $PRINTER -p 'odd=no even=yes' -F '*ManualFeed True' "$FILE"
