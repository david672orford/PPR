#
# mouse:~ppr/src/www/cgi_intl.pl
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 11 January 2002.
#

require "paths.ph";
defined($LOCALEDIR) || die;
defined($PACKAGE_PPRWWW) || die;

# What language will the user get if we don't turn on the translation routines?
my $UNTRANSLATED_LANGUAGE = "en";
my $UNTRANSLATED_CHARSET = "iso-8859-1";

# Which charsets for which languages?
my $CHARSET_DEFAULT = "iso-8859-1";
my %CHARSET_EXCEPTIONS = (
	"ru" => "koi8-r"
	);

# Did we suceed in loading the Perl modules for internationalization and did we
# select a language other than the default?
my $int_on = 0;

#
# This function should be called near the begining of the CGI script.  It
# returns the character set and the language code.  It makes its
# determination by parsing the "Accept-Language:" header and selecting
# one of the available languages.
#
sub cgi_intl_init
    {
    if(defined(my $langs = $ENV{HTTP_ACCEPT_LANGUAGE}))
	{
	my @result = eval
	    {
	    # Create a list of the available languages.
	    opendir(LDIR, $LOCALEDIR) || die $!;
	    my @langs_available = grep(!/\./, readdir(LDIR));
	    closedir(LDIR) || die $!;
	    push(@langs_available, $UNTRANSLATED_LANGUAGE);
	    print STDERR "Available languages: ", join(", ", @langs_available), "\n";

	    # Create a list of the user's language preferences.  Longer language
	    # ranges will be sorted to the end of the list in order to conform to
	    # RFC 2068 section 14.4 when we do the next step.
            my @lang_q_list = ();
            my $x = 0;
            foreach my $language_range (split(/\s*,\s*/, $langs))
                {
                if($language_range !~ /^([^;]+)(\s*;\s*q=([0-9\.]+))?\s*$/)
                    {
                    print STDERR "  Invalid language-range: $language_range\n";
                    next;
                    }
                my $lang = $1;
                my $q = defined($3) ? $3 : 1.0;
                push(@lang_q_list, "$lang $q $x");
		$x++;
                }
	    @lang_q_list = sort(@lang_q_list);
	    print STDERR "  Sorted language-ranges: ", join(", ", @lang_q_list), "\n";

	    # Apply the sorted rankings to the languages available.
	    my %lang_q_hash = ();
	    my %lang_tiebreaker = ();
	    my $default_q = 0.0;
	    foreach my $language_range (@lang_q_list)
	    	{
		my($lang, $q, $tiebreaker) = split(/ /, $language_range);
		if($lang eq "*")
		    {
		    print STDERR "    Default language q set to $q.\n";
		    $default_q = $q;
		    next;
		    }
		print STDERR "    Looking for matches for language range \"$lang\".\n";
		foreach my $matching_lang (grep(/^$lang(-.*)?$/, @langs_available))
		    {
		    print STDERR "      It matches language \"$matching_lang\", assigning q=$q.\n";
		    $lang_q_hash{$matching_lang} = $q;
		    $lang_tiebreaker{$matching_lang} = $tiebreaker;
		    }
	    	}

	    # Assign the default quality to all languages not mentioned in the
	    # "Accept-Language:" header.  If a language range of "*" was used,
	    # then uses its quaility value as the default quality.
	    foreach my $lang (@langs_available)
	    	{
		if(!defined($lang_q_hash{$lang}))
		    {
		    print STDERR "    Language $lang gets default q of $default_q.\n";
		    $lang_q_hash{$lang} = $default_q;
		    $lang_tiebreaker{$lang} = 1000;
		    }
	    	}

	    # Choose a language on the basis or rank.
	    my $selected_lang = undef;
	    my $highest_q = 0.0;
	    my $lowest_tiebreaker = 1000;
	    foreach my $lang (@langs_available)
	    	{
		my $q = $lang_q_hash{$lang};
		print STDERR "  Language $lang ranks $q";

		# If it has the highest quality ranking found so far,
		# then choose this language.  But, a later language may
		# displace this one.
		if($q > $highest_q)
		    {
		    print STDERR ", best so far";
		    $selected_lang = $lang;
		    $highest_q = $q;
		    $lowest_tiebreaker = $lang_tiebreaker{$lang};
		    }

		# If they are tied, RFC 2068 section 14.4 does not say how the tie
		# should be resolved.  Since Netscape Navigator 4.x puts the favoured
		# languages first (without quality values) we will give the victory to
		# the one that appeared first.
		elsif($q == $highest_q)
		    {
		    if($lang_tiebreaker{$lang} < $lowest_tiebreaker)
		    	{
		    	print STDERR ", tie broken favourably";
			$selected_lang = $lang;
			$highest_q = $q;
			$lowest_tiebreaker = $lang_tiebreaker{$lang};
			}
		    else
		    	{
		    	print STDERR ", tie broken unfavourably";
		    	}
		    }

		# Languages with a quality less than zero are not acceptable.
		elsif($q < 0.0)
		    {
		    print STDERR ", not acceptable";
		    }

		# This language was acceptable, but it has already lost out
		# to one with a higher quality value.
		else
		    {
		    print STDERR ", also ran";
		    }
		print STDERR ".\n";
	    	}

	    print STDERR "Selected language: ", defined($selected_lang) ? $selected_lang : "<default>", "\n";
	    if(defined($selected_lang) && $selected_lang ne $UNTRANSLATED_LANGUAGE)
		{
		# Put the selection into the normal environment variable where
		# setlocale() and the programs the CGI script runs can pick
		# it up.
		$ENV{LANG} = $selected_lang;

		# Many languages use Latin1, but there is a list of exceptions.
		my $selected_charset;
		if(!defined($selected_charset = $CHARSET_EXCEPTIONS{$selected_lang}))
		    {
		    $selected_charset = $CHARSET_DEFAULT;
		    }

		# Pull in the Perl modules needed for translation.
	    	require POSIX;
	    	import POSIX qw(locale_h);
		require Locale::gettext;
		import Locale::gettext;

		# Set things up so that gettext() will use the the language
		# the user has selected.
		setlocale(&LC_MESSAGES, "");
		bindtextdomain($PACKAGE_PPRWWW, $LOCALEDIR);
		textdomain($PACKAGE_PPRWWW);

		# Switch on translation in _() and tell the caller which
		# charset and language it should indicate in the HTTP header.
		$int_on = 1;
		return ($selected_charset, $selected_lang);
		}

	    return ($UNTRANSLATED_CHARSET, $UNTRANSLATED_LANGUAGE);
	    };
	if($@)
	    {
	    my $message = $@;
	    print STDERR "I18n error: $message";
	    }
	else
	    {
	    return @result;
	    }
	}

    # No "Accept-Language:" header, go with PPR's native language.
    return ($UNTRANSLATED_CHARSET, $UNTRANSLATED_LANGUAGE);
    }

#
# This returns the translation of the string first argument.
# Actually, if $int_on is false, it just returns its first
# argument.
#
sub _
    {
    my $text = shift;
    if($int_on && $text ne "")
	{
	#print STDERR "translating \"$text\"";
	$text = &gettext($text);
	#print STDERR " as \"$text\"\n";
	}
    return $text;
    }

#
# This function is used to wrap strings that xgettext should notice
# but on which we don't want to call gettext().  It will never do
# anything but return its argument.  It would be nice if we could
# eliminate it entirely since it serves no purpose at run-time.
#
sub N_
    {
    return shift;
    }

# This translates a string and HTML escapes it.
# (The html() function is in cgi_data.pl.)
sub H_
    {
    return &html(_(shift));
    }

# This ones does the same as H_ but converts spaces into non-breaking spaces.
sub H_NB_
    {
    return &html_nb(_(shift));
    }

#
# This function generates a <button> if a non-default language has
# been selected, otherwise it generates an <input type=submit> which
# doesn't permit the button label to differ from the value that is
# submitted when the button is pressed.
#
# Arguments:
#	* The value for name= property of the <input> or <button> tag.
#	* A translatable (but not yet translated) button label with the
#	  accesskey indicated by a proceding underscore.  An underscore
#	  should appear in the translation string if an accesskey is desired.
#	* Anything else to put inside the <input> or <button> tag.
#
sub isubmit
    {
    my($name, $value, $translation, $other) = @_;
    defined($name) && defined($value) && defined($translation) || die;
    my $accesskey = undef;

    # Try to get a Mozilla version number.
    my $mozilla_version = 0;
    if(defined($ENV{HTTP_USER_AGENT}) && $ENV{HTTP_USER_AGENT} =~ /^Mozilla\/(\d+\.\d+) /)
	{
	$mozilla_version = $1;
	}

    # <button> isn't implemented in Netscape 4.7 and doesn't work in IE 5.0,
    # so only enable it if we are using a Gecko Mozilla based browser.
    if($int_on && $mozilla_version >= 5.0)
    	{
	$accesskey = $1 if($translation =~ s/_(.)/$1/);
	$value =~ s/_//;
	$value = &html($value);
	print "<button type=\"submit\" name=\"$name\" value=\"$value\"";
	print " accesskey=\"$accesskey\"" if(defined($accesskey));
	print " ", $other if(defined($other));
	print ">", &html($translation), "</button>\n";
    	}

    else
	{
	$accesskey = $1 if($value =~ s/_(.)/$1/);
	$value = &html($value);
	print "<input type=\"submit\" name=\"$name\" value=\"$value\"";
	print " accesskey=\"$accesskey\"" if(defined($accesskey));
	print " ", $other if(defined($other));
	print ">\n";
	}
    }

1;
