mouse:~ppr/src/ppd/README.txt
10 July 1999.

This subdirectory contains PPD files that are part of PPR and are therefor
distributed under the same licensing terms as PPR.  PPD files from
Adobe and printer vendors are in ../vendors.

Those in the "ghostscript" subdirectory are for using various printers with
Ghostscript.  They are all built by running a master file through the
C preprocessor.

Those in the "generic" subdirectory are PPD files which describe imaginary
printers which sport features which are actually implemented by the PPR
spooler.

Those in the "other" directory are for printers that didn't have PPD files,
or for which PPD files with different option names were wanted, or PPD files
which work around printer bugs.  Some of these PPD files have "*Include:"
lines which include the vendor's PPD file or an Adobe PPD file.

The file "pagesizes.conf" is here because there wasn't a better place to
put it and because the names it equates to page sizes are the same names
as those used in PPD files.



