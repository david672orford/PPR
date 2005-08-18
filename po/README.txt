=============================================================================
 mouse:~ppr/src/po/README.txt
 mouse:~ppr/docs/misc/internationalization.txt (symbolic link)
 Last revised 17 August 2005.
=============================================================================

The po/ directory in the PPR distribution contains language translation files
for PPR.  PPR uses GNU Gettext to display program messages in languages other
than English.

If you are planing to start a new translation, please write to 
<David.Chappell@trincoll.edu> and tell him what language in order to make sure
nobody is working on it yet.  We would be happy to include more contributed 
translation files in future versions of PPR.

This README.txt explains how the Makefile in directory works.  It is 
recomended that you read the GNU Gettext manual too.

Basically, the GNU Gettext system uses files of these types:

.pot An extracted list of translatable program messages.  A .pot file
	will also contain spaces for the translations, but the spaces for the
	translation text will be empty.  These files are built automatically
	by extracting translatable strings from the source code.

.po	These files are finished translation files with the translation text
	filled in.

.mo	These are .po files that have been "compiled" into an indexed format
	which programs can used to lookup the translations of their messages.

=============================================================================
 Adding Translations for a New Language
=============================================================================

To start a new translation, you should first run "make pot" to make sure your
.pot files are up to date.  You should then make a copy of each .pot file with
the abbreviation for your language and a hyphen added at the begining and the
extesion changed to .po.  For example, for French, you would copy PPR.pot like
this:

	$ cp PPR.pot fr_FR-PPR.po

Do this for each .pot file.  You should then edit the newly created .po files,
filling in the empty translation strings.  But first, fill in the header at the
top.

Once that is done, edit the Makefile and add the abbreviation for the new
language to the "LANGUAGES=" list.  Then, run "make install" to install your
new translations.

=============================================================================
 Trying out the Translation
=============================================================================

Remember that the programs won't use your translations until you set the
LANG or LC_MESSAGES environment variable to the abbreviation for your
language.  If messages from pprd and the other daemons are to be translated,
then it will have to be started with LANG= set.  The System V style Init script
for PPR will start the daemon with LANG= set if it finds a setting for
"daemon lang =" in the [internationalization] section of ppr.conf.

=============================================================================
 Updating Translations for an Existing Language
=============================================================================

If you later want to update your translation, run "make pot"  and "make" to
create the lastest .pot files and merge them with the pre-existing .po files.
Translations that are no longer needed will be removed and template entries for
new translations which you must write will be added.  The merge program may
attempt to guess at the correct translation using fuzzy matching with other
translation strings.  It will mark such entries with the comment ",fuzzy".

=============================================================================

