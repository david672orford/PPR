#! /bin/sh

exec >.restore_symlinks
chmod 755 .restore_symlinks
echo "#! /bin/bash"

for link in `find . -type l`
    do
    contents=`readlink $link`

    # -h doesn't seem to work with some versions of bash.
    #echo "if [ ! -h $link ]; then ln -s $contents $link || exit 1; fi"
    echo "if [ ! -f $link ]; then ln -s $contents $link || exit 1; fi"

    done

exit 0
