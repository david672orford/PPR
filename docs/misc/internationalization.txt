=============================================================================
 mouse:~ppr/docs/misc/internationalization.txt
 mouse:~ppr/src/po/README.txt (symbolic link)
 Last revised 16 August 2002.
=============================================================================

The po/ directory in the PPR distribution contains language translation files
for PPR.  PPR uses GNU Gettext to display program messages in languages other
than English.

So far we have a partial translation into French.  If you are going
to make a new translation, please write to "David.Chappell@mail.trincoll.edu"
to make sure a translation isn't already in progress.  I would be happy to
include more contributed translation files in future versions of PPR.

These instructions explain the shell scripts and Makefile I provide.  I
recomend that you read the GNU Gettext manual too.

Basically, the GNU Gettext system uses files of these types:

.pot	An extracted list of translatable program messages.  A .pot file
	will also contain spaces for the translations, but the spaces for the
	translation text will be empty.

.po	These files are finished translation files with the translation text
	filled in.

.pox	These files are work files.  They are formed by merging the current
	.pot files and .po files prepared for a previous version of PPR.
	If there are new or changed messages, then some of the spaces for
	translation text will be empty.  The translator fills in the missing
	translations and then renames the .pox file to .po.

.mo	These are .po files that have been "compiled" into an indexed format
	which programs can used to lookup the translations of their messages.

=============================================================================
 Running extract_to_pot.sh
=============================================================================

This file runs xgettext on the PPR source code.  This process extracts
a list of those message strings which have been marked as tranlatable.
This list is used to generate a series of .pot files.

=============================================================================
 Adding Translations for a New Language
=============================================================================

To start a new translation, you should first run extract_to_pot.sh to make
sure your .pot files are up to date.

You should then run merge_to_pox.sh.  The single argument should be the
abbreviation for your language, for example, "fr" (French) or "ru"
(Russian).  Since, if you are starting a new translation, there are no
existing translation files (.po files) to merge, merge_to_pox.sh will simply
copy the .pot files to .pox files.  The .pox extension indicates that this
is the copy of the translation file that you are working on.

You should then edit the .pox files, filling in the empty translation strings.

Once your are done editing, change the extensions of the .pox files to ".po".
The script move_pox_to_po.sh will do this for you if you run it with the
language abbrebiation on the command line.

Once that is done, edit the Makefile and add the abbreviation for the new
language to the "LANGUAGES=" list.  Then, run "make install" to install your
new translations.

Remember that the programs won't use your translations until you set the
LANG or LC_MESSAGES environment variable to the abbreviation for your
language.  If messages from pprd and the other daemons are to be translated,
then it will have to be started with LANG= set.  The System V style Init script
for PPR will start the daemon with LANG= set if it finds a setting for
"daemon lang =" in the [internationalization] section of ppr.conf.

If you later want to update your translation, follow the same steps as for a
new translation.  The merge_to_pox.sh script will read in each of your .po
files and the cooresponding .pox file.  It will remove translations that are
no longer needed and add template entries for new translations which you must
write.  It may attempt to guess at the correct translation using fuzzy matching
with other translation strings.  It will mark such entries with the comment
",fuzzy".

=============================================================================
 Updating Translations for an Existing Language
=============================================================================

If you are the maintainer of an existing translation and want to update it
for new messages in PPR then you should run extract_to_pot.sh and
merge_to_pox.sh as described above.  The resulting .pox files should include
your old translations.  Any which translate messages which no longer exist or
have changed will have been moved to the bottom of the file and turned into
comments.  Any new messages will have empty translation strings.  You should
make any necessary revisions and then rename the .pox file, replacing the
previous .po file.  You can then run "make install" as described above.

=============================================================================

