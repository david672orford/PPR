#
# mouse:~ppr/src/misc/cgi_data.pl
# Copyright 1995--2003, Trinity College Computing Center.
# Written by David Chappell.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Last modified 17 December 2003.
#

#
# Read the data QUERY_STRING data, and if the method is POST, the
# data on stdin.  The data is decoded and stored in the associative
# array %data.
#
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
		return;
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

#
# Return an item from the CGI data and
# clear it so that it will only be in
# the submitted data if we have it in
# the current form.
#
sub cgi_data_move
	{
	my $name = shift;
	my $default = shift;
	my $value = $data{$name};
	delete($data{$name});
	return defined($value) ? $value : $default;
	}

#
# This function is just like cgi_data_move(), except it doesn't
# remove the value from %data.
#
sub cgi_data_peek
	{
	my $name = shift;
	my $default = shift;
	my $value = $data{$name};
	return defined($value) ? $value : $default;
	}

#
# Emmit the data gathered on other pages as hidden
# form fields.
#
sub cgi_write_data
	{
	my $datum;
	my $value;
	foreach $datum (sort(keys %data))
		{
		$value = $data{$datum};
		$value =~ s/"/&quot;/g;
		print "<INPUT TYPE=hidden NAME=\"$datum\" VALUE=\"$value\">\n";
		}
	}

# Emit the form submission data again so that humans can read it
# easily.  It will normally be off the bottom of the page so the
# user won't see it unless he scrolls down.
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

#
# This function takes a text string and encodes anything that would have
# special meaning in HTML.
#
# We need to do this in a function, because a translation might
# contain a character such as <, >, &, or something we haven't
# dreamed of.  It would be a pain to go back and modify hundreds of
# lines of code.
#
sub html
	{
	my $text = shift;
	$text =~ s/&/&amp;/g;		# This one must be first.
	$text =~ s/</&lt;/g;
	$text =~ s/>/&gt;/g;
	return $text;
	}

#
# This one goes a step furthur and converts spaces to non-breaking spaces. 
# This is a good thing to use with table cells we don't want broken.
#
sub html_nb
	{
	my $text = html(shift);
	$text =~ s/ /&nbsp;/g;

	return $text;
	}

#
# Return the argument encoded as an HTML attribute value.
# This differs from html() in that it also quotes ASCII
# double quotes and it enclose the whole thing in ASCII
# double quotes.
#
sub html_value
	{
	my $text = html(shift);
	$text =~ s/"/&quot;/g;
	return "\"$text\"";
	}

#
# This function applies form encoding to the name=value pair supplied.
#
sub form_urlencoded
	{
	my($name, $value) = @_;
	$name =~ s/([^a-zA-Z0-9 ])/sprintf("%%%02X",unpack("C",$1))/ge;
	$name =~ s/ /+/g;
	$value =~ s/([^a-zA-Z0-9 ])/sprintf("%%%02X",unpack("C",$1))/ge;
	$value =~ s/ /+/g;
	return "$name=$value";
	}

#
# Return the argument encoded as a Javascript string suitable for inclusion
# in an HTML attribute value.
#
sub javascript_string
	{
	my $text = html(shift);
	$text =~ s/'/\\'/g;
	return "'$text'";
	}

#
# If the value is undefined, convert it to an empty string.
#
sub undef_to_empty
	{
	my $value = shift;
	return $value if defined $value;
	return "";
	}

1;

