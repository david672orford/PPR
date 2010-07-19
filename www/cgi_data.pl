#
# mouse:~ppr/src/misc/cgi_data.pl
# Copyright 1995--2010, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 28 April 2010.
#

=head1 cgi_data.pl

This library handles CGI POST data and GET or POST query strings.

=over 4

=cut

=item cgi_read_data()

Read the data QUERY_STRING data, and if the method is POST, the
data on stdin.  The data is decoded and stored in the associative
array %data.

=cut
sub cgi_read_data
	{
	undef %data;

	# This is so as not to throw misleading unitialized variable warnings
	# during testing.  We set a few environment variables in order to
	# suppress other warnings.
	if(!defined($ENV{REQUEST_METHOD}))
		{
		print STDERR "Warning: not running in a CGI context.\n";
		$ENV{SCRIPT_NAME} = $0;
		$ENV{REMOTE_USER} = (getpwuid($<))[0];
		$ENV{SERVER_NAME} = "placeholder";
		$ENV{SERVER_PORT} = 80;
		$ENV{REQUEST_METHOD} = "GET" unless(defined($ENV{QUERY_STRING}));
		$ENV{QUERY_STRING} = "" unless(defined($ENV{QUERY_STRING}));
		}

	# If the request method is POST, get the data on STDIN.
	my $post_data = "";
	if($ENV{REQUEST_METHOD} eq "POST")
		{
		read(STDIN, $post_data, $ENV{CONTENT_LENGTH});
		}

	# Split into name=value pairs.
	my $item;
	foreach $item (split(/[&;]/, $ENV{QUERY_STRING}), split(/&/, $post_data))
		{
		# spaces are encoded as plus signs
		$item =~ s/\+/ /g;

		my($name, $value) = split(/=/, $item);

		# Non-alphanumberic characters in the name or value are
		# encoded as %HH where HH is two hexadecimal digits.
		$name =~ s/%([0-9A-Fa-f]{2})/sprintf("%c",hex($1))/ge;
		$value =~ s/%([0-9A-Fa-f]{2})/sprintf("%c",hex($1))/ge;

		#print STDERR "\$name=\"$name\", \$value=\"$value\"\n";

		# Store the value in the associative array.	 If a variable
		# with this name is already there, append a space and the
		# new value to make a crude sort of list.  
		if(! defined($data{$name}) || $data{$name} eq "")
			{ $data{$name} = $value }
		else
			{ $data{$name} .= " $value" }
		}
	}

=item cgi_data_move()

Return an item from the CGI data and clear it so that it will only be in the
submitted data if we have it in the current form.  The first parameter is the
item to be returned, the second is the default value to be returned if the
item is not found.

=cut
sub cgi_data_move
	{
	my $name = shift;
	my $default = shift;
	my $value = $data{$name};
	delete($data{$name});
	return defined($value) ? $value : $default;
	}

=item cgi_data_peek()

This function is just like cgi_data_move(), except it doesn't
remove the value from %data.

=cut
sub cgi_data_peek
	{
	my $name = shift;
	my $default = shift;
	my $value = $data{$name};
	return defined($value) ? $value : $default;
	}

=item cgi_write_data()

Emmit any items remaining in %data as hidden form fields.

=cut
sub cgi_write_data
	{
	my $datum;
	my $value;
	foreach $datum (sort(keys %data))
		{
		$value = $data{$datum};
		if(!defined($value))
			{
			print STDERR "Warning: CGI datum \"$datum\" has a null value.\n";
			$value = "";
			}
		$value =~ s/"/&quot;/g;
		print "<INPUT TYPE=hidden NAME=\"$datum\" VALUE=\"$value\">\n";
		}
	}

=item cgi_debug_data()

Print the form submission %data has as HTML so that humans can read it
easily.  It will normally be off the bottom of the page so the
user won't see it unless he scrolls down.

=cut
sub cgi_debug_data()
	{
	my $x;
	for($x=0; $x < 15; $x++)
		{ print "<br>\n" }

	print "<pre>\n";

	my $datum;
	foreach $datum (sort(keys %data))
		{ print "$datum=\"$data{$datum}\"\n" }

	print "</pre>\n";
	}

=item html()

This function takes a text string and encodes anything that would have
special meaning in HTML.

We need to do this in a function, because a translation might
contain a character such as <, >, &, or something we haven't
dreamed of.  It would be a pain to go back and modify hundreds of
lines of code.

=cut
sub html
	{
	my $text = shift;
	$text =~ s/&/&amp;/g;		# This one must be first.
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	return $text;
	}

=item html_nb()

This one goes a step furthur and converts spaces to non-breaking spaces. 
This is a good thing to use with table cells we don't want broken.

=cut
sub html_nb
	{
	my $text = html(shift);
	$text =~ s/ /&nbsp;/g;

	return $text;
	}

=item html_value()

Return the argument encoded as an HTML attribute value.
This differs from html() in that it also quotes ASCII
double quotes and it enclose the whole thing in ASCII
double quotes.

=cut
sub html_value
	{
	my $text = html(shift);
	$text =~ s/"/&quot;/g;
	return "\"$text\"";
	}

=item form_urlencoded()

This function applies form encoding to the name=value pair supplied.

=cut
sub form_urlencoded
	{
	my($name, $value) = @_;
	$name =~ s/([^a-zA-Z0-9 ])/sprintf("%%%02X",unpack("C",$1))/ge;
	$name =~ s/ /+/g;
	$value =~ s/([^a-zA-Z0-9 ])/sprintf("%%%02X",unpack("C",$1))/ge;
	$value =~ s/ /+/g;
	return "$name=$value";
	}

=item javascript_string()

Return the argument encoded as a Javascript string suitable for inclusion
in an HTML attribute value.

=cut
sub javascript_string
	{
	my $text = html(shift);
	$text =~ s/'/\\'/g;
	return "'$text'";
	}

=item undef_to_empty()

If the argument is undefined, return it to an empty string,
otherwise return the argument.

=cut
sub undef_to_empty
	{
	my $value = shift;
	return $value if defined $value;
	return "";
	}

=item print_stripped()

Strip whitespace from the begining of lines in the argument and print the
argument to the  indicated file handle .  We use this so that we can indent
here-documents without the indenting appearing in the final file.

=cut
sub print_stripped
    {
    my($fh, $string) = @_;
    $string =~ s/^[ \t]+//g;
    $string =~ s/\n[ \t]+/\n/g;
    print $fh $string;
    }

=back

=cut
1;

